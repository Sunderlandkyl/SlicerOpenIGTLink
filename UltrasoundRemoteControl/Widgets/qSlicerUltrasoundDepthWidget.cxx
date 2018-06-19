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

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundDepthWidget.h"
#include "ui_qSlicerUltrasoundDepthWidget.h"

#include "vtkSlicerOpenIGTLinkCommand.h"
#include "vtkMRMLIGTLConnectorNode.h"

#include <vtkXMLUtilities.h>
#include <vtkVariant.h>

const std::string PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID = "US_Remote";

//-----------------------------------------------------------------------------
class qSlicerUltrasoundDepthWidgetPrivate
{
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDepthWidget);

protected:
  qSlicerUltrasoundDepthWidget* const q_ptr;
public:
  qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& object);
  ~qSlicerUltrasoundDepthWidgetPrivate();
  void init();

public: 
  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> CmdSetParameter;
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> ConnectorNode;
  std::string DeviceID = "";

  QHBoxLayout *horizontalLayout;
  QLabel *label;
  ctkSliderWidget *depthSlider;

  virtual void setupUi(QWidget *qSlicerUltrasoundDepthWidget);
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidgetPrivate::qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& object)
  : q_ptr(&object)
  , CmdSetParameter(vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New())
{
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidgetPrivate::~qSlicerUltrasoundDepthWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundDepthWidget);
}

void qSlicerUltrasoundDepthWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundDepthWidget);
  this->setupUi(q);

  this->CmdSetParameter->SetCommandName("SetUsParameter");
  this->CmdSetParameter->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);

  QObject::connect(this->depthSlider, SIGNAL(valueChanged(double)), q, SLOT(setParameter()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidgetPrivate::setupUi(QWidget *qSlicerUltrasoundDepthWidget)
{
  if (qSlicerUltrasoundDepthWidget->objectName().isEmpty())
  {
    qSlicerUltrasoundDepthWidget->setObjectName(QStringLiteral("qSlicerUltrasoundDepthWidget"));
  }

  qSlicerUltrasoundDepthWidget->resize(388, 44);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundDepthWidget->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundDepthWidget->setSizePolicy(sizePolicy);
  qSlicerUltrasoundDepthWidget->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundDepthWidget);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  label = new QLabel(qSlicerUltrasoundDepthWidget);
  label->setObjectName(QStringLiteral("label"));
  label->setText("Depth (mm):");

  horizontalLayout->addWidget(label);

  depthSlider = new ctkSliderWidget(qSlicerUltrasoundDepthWidget);
  depthSlider->setObjectName(QStringLiteral("depthSlider"));
  depthSlider->setMaximum(200);
  depthSlider->setTracking(false);

  horizontalLayout->addWidget(depthSlider);

  QMetaObject::connectSlotsByName(qSlicerUltrasoundDepthWidget);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDepthWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidget::qSlicerUltrasoundDepthWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qSlicerUltrasoundDepthWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->init();
  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidget::~qSlicerUltrasoundDepthWidget()
{
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDepthWidget::getDepthMM()
{
  Q_D(qSlicerUltrasoundDepthWidget);
  return d->depthSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::setDepthMM(double depth)
{
  Q_D(qSlicerUltrasoundDepthWidget);
  return d->depthSlider->setValue(depth);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::onConnectionChanged()
{
  Q_D(qSlicerUltrasoundDepthWidget);
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
void qSlicerUltrasoundDepthWidget::onConnected()
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->depthSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::onDisconnected()
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->depthSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::setParameter()
{
  Q_D(qSlicerUltrasoundDepthWidget);

  if (!d->ConnectorNode)
  {
    return;
  }

  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  rootElement->SetAttribute("Name", "SetUsParameter");
  rootElement->SetAttribute("UsDeviceId", "VideoDevice"); //TODO: currently hardcoded
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", "DepthMm");
  nestedElement->SetAttribute("Value", vtkVariant(this->getDepthMM()).ToString());
  rootElement->AddNestedElement(nestedElement);

  std::stringstream ss;
  vtkXMLUtilities::FlattenElement(rootElement, ss);

  d->CmdSetParameter->SetCommandText(ss.str());
  d->ConnectorNode->SendCommand(d->CmdSetParameter);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::updateParameter()
{
  

}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::onUpdate()
{

}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::setConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  Q_D(qSlicerUltrasoundDepthWidget);

  this->qvtkReconnect(d->ConnectorNode, node, vtkCommand::ModifiedEvent, this, SLOT(onConnectionChanged()));
  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(onConnectionChanged()));
  this->qvtkReconnect(d->ConnectorNode, node, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(onConnectionChanged()));
  d->ConnectorNode = node;
  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* qSlicerUltrasoundDepthWidget::getConnectorNode()
{
  Q_D(qSlicerUltrasoundDepthWidget);
  return d->ConnectorNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::setDeviceID(std::string deviceID)
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->DeviceID = deviceID;
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundDepthWidget::getDeviceID()
{
  Q_D(qSlicerUltrasoundDepthWidget);
  return d->DeviceID;
}
