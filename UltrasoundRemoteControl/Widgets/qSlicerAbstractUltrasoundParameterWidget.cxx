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

#include "vtkSlicerOpenIGTLinkCommand.h"

// VTK includes
#include <vtkXMLUtilities.h>

const std::string PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID = "US_Remote";

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidgetPrivate::qSlicerAbstractUltrasoundParameterWidgetPrivate(qSlicerAbstractUltrasoundParameterWidget *q)
  : q_ptr(q)
  , ParameterName("")
  , ParameterUnit("")
  , CmdSetParameter(vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New())
  , CmdGetParameter(vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New())
  , ConnectorNode(NULL)
{
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidgetPrivate::init()
{
  Q_Q(qSlicerAbstractUltrasoundParameterWidget);
  this->CmdSetParameter->SetCommandName("SetUsParameter");
  this->CmdSetParameter->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);
  this->CmdSetParameter->BlockingOff();
  this->CmdSetParameter->SetCommandTimeoutSec(1.0);
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(QWidget* parentWidget)
  : qMRMLWidget(parentWidget)
  , d_ptr(new qSlicerAbstractUltrasoundParameterWidgetPrivate(this))
{
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(qSlicerAbstractUltrasoundParameterWidgetPrivate* d)
  : d_ptr(d)
{
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
  
  if (!d->ConnectorNode)
  {
    return;
  }

  std::string parameterValue = this->getParameterValue();
  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "SetUsParameter");
  rootElement->SetAttribute("UsDeviceId", "VideoDevice"); //TODO: currently hardcoded
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", d->ParameterName.c_str());
  nestedElement->SetAttribute("Value", parameterValue.c_str());
  rootElement->AddNestedElement(nestedElement);
  
  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);
  
  d->CmdSetParameter->SetCommandText(ss.str());
  d->ConnectorNode->SendCommand(d->CmdSetParameter);
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::getUltrasoundParameter()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
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

////-----------------------------------------------------------------------------
//std::string qSlicerAbstractUltrasoundParameterWidget::parameterUnit()
//{
//  Q_D(qSlicerAbstractUltrasoundParameterWidget);
//  return d->ParameterUnit;
//}
//
////-----------------------------------------------------------------------------
//void qSlicerAbstractUltrasoundParameterWidget::setParameterUnit(std::string unit)
//{
//  Q_D(qSlicerAbstractUltrasoundParameterWidget);
//  d->ParameterUnit = unit;
//}