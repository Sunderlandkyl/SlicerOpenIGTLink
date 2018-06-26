/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

// OpenIGTLinkIO includes
#include <igtlioCommand.h>

// VTK includes
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidgetPrivate::qSlicerAbstractUltrasoundParameterWidgetPrivate(qSlicerAbstractUltrasoundParameterWidget *q)
  : q_ptr(q)
  , ParameterName("")
  , ParameterUnit("")
  , CmdSetParameter(igtlioCommandPointer::New())
  , CmdGetParameter(igtlioCommandPointer::New())
  , ConnectorNode(NULL)
{
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidgetPrivate::init()
{
  Q_Q(qSlicerAbstractUltrasoundParameterWidget);
  this->CmdSetParameter->SetName("SetUsParameter");
  this->CmdSetParameter->BlockingOff();
  this->CmdSetParameter->SetTimeoutSec(1.0);

  this->CmdGetParameter->SetName("GetUsParameter");
  this->CmdGetParameter->BlockingOff();
  this->CmdGetParameter->SetTimeoutSec(1.0);
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(qSlicerAbstractUltrasoundParameterWidgetPrivate* d)
  : d_ptr(d)
{
  connect(&d->PeriodicParameterTimer, SIGNAL(timeout()), this, SLOT(checkActualValue()));
  this->qvtkConnect(d->CmdSetParameter, igtlioCommand::CommandCompletedEvent, this, SLOT(onSetUltrasoundParameterCompleted()));
  this->qvtkConnect(d->CmdGetParameter, igtlioCommand::CommandCompletedEvent, this, SLOT(onGetUltrasoundParameterCompleted()));
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::~qSlicerAbstractUltrasoundParameterWidget()
{
}

//-----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* qSlicerAbstractUltrasoundParameterWidget::getConnectorNode()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->ConnectorNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  this->qvtkReconnect(d->ConnectorNode, node, vtkCommand::ModifiedEvent, this, SLOT(onConnectionChanged()));
  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(onConnectionChanged()));
  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(onConnectionChanged()));
  
  d->ConnectorNode = node;
  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onConnectionChanged()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  if (d->ConnectorNode && d->ConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::StateConnected)
  {
    this->onConnected();
    this->getUltrasoundParameter();
  }
  else
  {
    this->onDisconnected();
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setDeviceID(std::string deviceID)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->DeviceID = deviceID;
}

//-----------------------------------------------------------------------------
std::string qSlicerAbstractUltrasoundParameterWidget::getDeviceID()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->DeviceID;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setUltrasoundParameter()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  
  if (!d->ConnectorNode || d->ConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateConnected)
  {
    return;
  }

  std::string expectedParameterValue = this->getExpectedParameterValue();
  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "SetUsParameter");
  rootElement->SetAttribute("UsDeviceId", "VideoDevice"); //TODO: currently hardcoded
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", d->ParameterName.c_str());
  nestedElement->SetAttribute("Value", expectedParameterValue.c_str());
  rootElement->AddNestedElement(nestedElement);

  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);
  
  d->CmdSetParameter->SetCommandContent(ss.str());
  d->ConnectorNode->SendCommand(d->CmdSetParameter);

}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::getUltrasoundParameter()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  if (!d->ConnectorNode || d->ConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateConnected)
  {
    return;
  }

  std::string expectedParameterValue = this->getExpectedParameterValue();
  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "GetUsParameter");
  rootElement->SetAttribute("UsDeviceId", "VideoDevice"); //TODO: currently hardcoded
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", d->ParameterName.c_str());
  rootElement->AddNestedElement(nestedElement);

  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);

  d->CmdGetParameter->SetCommandContent(ss.str());
  d->ConnectorNode->SendCommand(d->CmdGetParameter);

}

//-----------------------------------------------------------------------------
const char* qSlicerAbstractUltrasoundParameterWidget::parameterName()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->ParameterName.c_str();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setParameterName(const char* parameterName)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  if (parameterName)
  {
    d->ParameterName = parameterName;
  }
  else
  {
    d->ParameterName = "";
  }
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onSetUltrasoundParameterCompleted()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);

  if (d->CmdSetParameter->GetSuccessful())
  {
    this->setActualParameterValue(this->getExpectedParameterValue());
  }

  this->setParameterCompleted();

  //if (this->getActualParameterValue() != this->getExpectedParameterValue())
  //{
  //  d->PeriodicParameterTimer.start(2000);
  //}
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::onGetUltrasoundParameterCompleted()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  std::cout << d->CmdGetParameter->GetResponseContent() << std::endl;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::checkActualValue()
{
  //Q_D(qSlicerAbstractUltrasoundParameterWidget);

  //std::string expectedValue = this->getExpectedParameterValue();
  //std::string actualValue = this->getActualParameterValue();

  //if (expectedValue == actualValue || d->ConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateConnected)
  //{
  //  d->PeriodicParameterTimer.stop();
  //  return;
  //}

  //// Maybe should get instead?
  //this->setUltrasoundParameter();
}