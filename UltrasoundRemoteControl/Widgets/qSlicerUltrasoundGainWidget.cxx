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
#include "qSlicerUltrasoundGainWidget.h"
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
class qSlicerUltrasoundGainWidgetPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundGainWidget);

  qSlicerUltrasoundGainWidgetPrivate(qSlicerUltrasoundGainWidget& p);
  ~qSlicerUltrasoundGainWidgetPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget *qSlicerUltrasoundGainWidget);

protected:
  qSlicerUltrasoundGainWidget* const q_ptr;

public:
  QHBoxLayout *horizontalLayout;
  QLabel *label;
  ctkSliderWidget *gainSlider;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundGainWidgetPrivate::qSlicerUltrasoundGainWidgetPrivate(qSlicerUltrasoundGainWidget& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
  this->ParameterName = "GainPercent";
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundGainWidgetPrivate::~qSlicerUltrasoundGainWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundGainWidget);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundGainWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundGainWidget);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->gainSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundGainWidgetPrivate::setupUi(QWidget *qSlicerUltrasoundGainWidget)
{
  if (qSlicerUltrasoundGainWidget->objectName().isEmpty())
  {
    qSlicerUltrasoundGainWidget->setObjectName(QStringLiteral("qSlicerUltrasoundGainWidget"));
  }

  qSlicerUltrasoundGainWidget->resize(388, 44);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundGainWidget->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundGainWidget->setSizePolicy(sizePolicy);
  qSlicerUltrasoundGainWidget->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundGainWidget);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  label = new QLabel(qSlicerUltrasoundGainWidget);
  label->setObjectName(QStringLiteral("label"));
  label->setText("Gain (%):");

  horizontalLayout->addWidget(label);

  gainSlider = new ctkSliderWidget(qSlicerUltrasoundGainWidget);
  gainSlider->setObjectName(QStringLiteral("gainSlider"));
  gainSlider->setMinimum(0);
  gainSlider->setMaximum(100);
  gainSlider->setTracking(false);

  horizontalLayout->addWidget(gainSlider);

  QMetaObject::connectSlotsByName(qSlicerUltrasoundGainWidget);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundGainWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundGainWidget::qSlicerUltrasoundGainWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundGainWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundGainWidget);
  d->init();
  this->qvtkConnect(d->CmdSetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(setUltrasoundParameterCompleted));
  this->qvtkConnect(d->CmdGetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(getUltrasoundParameterCompleted));

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundGainWidget::~qSlicerUltrasoundGainWidget()
{
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundGainWidget::getGainPercent()
{
  Q_D(qSlicerUltrasoundGainWidget);
  return d->gainSlider->value();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundGainWidget::setGainPercent(double gain)
{
  Q_D(qSlicerUltrasoundGainWidget);
  return d->gainSlider->setValue(gain);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundGainWidget::onConnected()
{
  Q_D(qSlicerUltrasoundGainWidget);
  d->gainSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundGainWidget::onDisconnected()
{
  Q_D(qSlicerUltrasoundGainWidget);
  d->gainSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundGainWidget::getParameterValue()
{
  return vtkVariant(this->getGainPercent()).ToString();
}