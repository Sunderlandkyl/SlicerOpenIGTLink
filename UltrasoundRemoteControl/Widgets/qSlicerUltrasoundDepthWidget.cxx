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

#include <vtkXMLUtilities.h>

const std::string PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID = "US_Remote";

//-----------------------------------------------------------------------------
class qSlicerUltrasoundDepthWidgetPrivate : public Ui_qSlicerUltrasoundDepthWidget, public qSlicerAbstractUltrasoundParameterWidgetPrivate
{
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDepthWidget);

public:
  qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& object);
  ~qSlicerUltrasoundDepthWidgetPrivate();
  void init();
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidgetPrivate::qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& q)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&q) 
{
  std::cout << "TEST" << std::endl;
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

  this->CmdSetParameter->SetCommandName("SET_US_PARAMETER_CMD");

  //QObject::connect(this->LauncherConnectCheckBox, SIGNAL(toggled(bool)), q, SLOT(onConnectCheckBoxChanged(bool)));
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDepthWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidget::qSlicerUltrasoundDepthWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(_parent, *new qSlicerUltrasoundDepthWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidget::qSlicerUltrasoundDepthWidget(QWidget* _parent, qSlicerUltrasoundDepthWidgetPrivate &d)
  : qSlicerAbstractUltrasoundParameterWidget(_parent, d)
{
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
void qSlicerUltrasoundDepthWidget::onConnected()
{

}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::onDisconnected()
{

}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidget::setParameter()
{
  Q_D(qSlicerUltrasoundDepthWidget);

  vtkNew<vtkXMLDataElement> rootElement;
  rootElement->SetName("Command");
  vtkNew<vtkXMLDataElement> nestedElement;
  nestedElement->SetName("Parameter");
  nestedElement->SetAttribute("Name", "ImagingDepthMm");
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
