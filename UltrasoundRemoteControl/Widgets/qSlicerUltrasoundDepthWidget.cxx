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
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkSlicerOpenIGTLinkCommand.h"
#include "vtkMRMLIGTLConnectorNode.h"

#include <vtkXMLUtilities.h>
#include <vtkVariant.h>

// Qt includes
#include <QHBoxLayout>
#include <QLabel>

// ctk includes
#include <ctkSliderWidget.h>

//-----------------------------------------------------------------------------
class qSlicerUltrasoundDepthWidgetPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDepthWidget);

  qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& p);
  ~qSlicerUltrasoundDepthWidgetPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget *qSlicerUltrasoundDepthWidget);

protected:
  qSlicerUltrasoundDepthWidget* const q_ptr;

public:
  QHBoxLayout *horizontalLayout;
  QLabel *label;
  ctkSliderWidget *depthSlider;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidgetPrivate::qSlicerUltrasoundDepthWidgetPrivate(qSlicerUltrasoundDepthWidget& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
  this->ParameterName = "DepthMm";
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidgetPrivate::~qSlicerUltrasoundDepthWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundDepthWidget);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDepthWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundDepthWidget);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->depthSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
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
  depthSlider->setMinimum(10);
  depthSlider->setMaximum(150);
  depthSlider->setTracking(false);

  horizontalLayout->addWidget(depthSlider);

  QMetaObject::connectSlotsByName(qSlicerUltrasoundDepthWidget);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDepthWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDepthWidget::qSlicerUltrasoundDepthWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundDepthWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundDepthWidget);
  d->init();
  this->qvtkConnect(d->CmdSetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(setUltrasoundParameterCompleted));
  this->qvtkConnect(d->CmdGetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(getUltrasoundParameterCompleted));

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
std::string qSlicerUltrasoundDepthWidget::getParameterValue()
{
  return vtkVariant(this->getDepthMM()).ToString();
}