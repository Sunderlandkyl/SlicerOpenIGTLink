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
#include "qSlicerUltrasoundDynamicRangeWidget.h"
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
class qSlicerUltrasoundDynamicRangeWidgetPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDynamicRangeWidget);

  qSlicerUltrasoundDynamicRangeWidgetPrivate(qSlicerUltrasoundDynamicRangeWidget& p);
  ~qSlicerUltrasoundDynamicRangeWidgetPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget *qSlicerUltrasoundDynamicRangeWidget);

protected:
  qSlicerUltrasoundDynamicRangeWidget* const q_ptr;

public:
  QHBoxLayout *horizontalLayout;
  QLabel *label;
  ctkSliderWidget *dynamicRangeSlider;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDynamicRangeWidgetPrivate::qSlicerUltrasoundDynamicRangeWidgetPrivate(qSlicerUltrasoundDynamicRangeWidget& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
  this->ParameterName = "DynRangeDb";
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDynamicRangeWidgetPrivate::~qSlicerUltrasoundDynamicRangeWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundDynamicRangeWidget);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDynamicRangeWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundDynamicRangeWidget);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->dynamicRangeSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDynamicRangeWidgetPrivate::setupUi(QWidget *qSlicerUltrasoundDynamicRangeWidget)
{
  if (qSlicerUltrasoundDynamicRangeWidget->objectName().isEmpty())
  {
    qSlicerUltrasoundDynamicRangeWidget->setObjectName(QStringLiteral("qSlicerUltrasoundDynamicRangeWidget"));
  }

  qSlicerUltrasoundDynamicRangeWidget->resize(388, 44);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundDynamicRangeWidget->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundDynamicRangeWidget->setSizePolicy(sizePolicy);
  qSlicerUltrasoundDynamicRangeWidget->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundDynamicRangeWidget);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  label = new QLabel(qSlicerUltrasoundDynamicRangeWidget);
  label->setObjectName(QStringLiteral("label"));
  label->setText("Dynamic range (dB):");

  horizontalLayout->addWidget(label);

  dynamicRangeSlider = new ctkSliderWidget(qSlicerUltrasoundDynamicRangeWidget);
  dynamicRangeSlider->setObjectName(QStringLiteral("dynamicRangeSlider"));
  dynamicRangeSlider->setPageStep(10);
  dynamicRangeSlider->setSingleStep(1);
  dynamicRangeSlider->setMinimum(10);
  dynamicRangeSlider->setMaximum(100);
  dynamicRangeSlider->setTracking(false);

  horizontalLayout->addWidget(dynamicRangeSlider);

  QMetaObject::connectSlotsByName(qSlicerUltrasoundDynamicRangeWidget);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDynamicRangeWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDynamicRangeWidget::qSlicerUltrasoundDynamicRangeWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundDynamicRangeWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundDynamicRangeWidget);
  d->init();
  this->qvtkConnect(d->CmdSetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(setUltrasoundParameterCompleted));
  this->qvtkConnect(d->CmdGetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(getUltrasoundParameterCompleted));

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDynamicRangeWidget::~qSlicerUltrasoundDynamicRangeWidget()
{
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDynamicRangeWidget::getDynamicRangeDb()
{
  Q_D(qSlicerUltrasoundDynamicRangeWidget);
  return d->dynamicRangeSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDynamicRangeWidget::setDynamicRangeDb(double dynamicRange)
{
  Q_D(qSlicerUltrasoundDynamicRangeWidget);
  return d->dynamicRangeSlider->setValue(dynamicRange);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDynamicRangeWidget::onConnected()
{
  Q_D(qSlicerUltrasoundDynamicRangeWidget);
  d->dynamicRangeSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDynamicRangeWidget::onDisconnected()
{
  Q_D(qSlicerUltrasoundDynamicRangeWidget);
  d->dynamicRangeSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundDynamicRangeWidget::getParameterValue()
{
  return vtkVariant(this->getDynamicRangeDb()).ToString();
}