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
#include "qSlicerUltrasoundFrequencyWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkSlicerOpenIGTLinkCommand.h"
#include "vtkMRMLIGTLConnectorNode.h"

#include <vtkXMLUtilities.h>
#include <vtkVariant.h>

// Qt includes
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>

// ctk includes
#include <ctkSliderWidget.h>

//-----------------------------------------------------------------------------
class qSlicerUltrasoundFrequencyWidgetPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundFrequencyWidget);

  qSlicerUltrasoundFrequencyWidgetPrivate(qSlicerUltrasoundFrequencyWidget& p);
  ~qSlicerUltrasoundFrequencyWidgetPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget *qSlicerUltrasoundFrequencyWidget);

protected:
  qSlicerUltrasoundFrequencyWidget* const q_ptr;

public:
  QHBoxLayout *horizontalLayout;
  QLabel *label;
  ctkSliderWidget *frequencySlider;
  //QComboBox *frequencySelector;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundFrequencyWidgetPrivate::qSlicerUltrasoundFrequencyWidgetPrivate(qSlicerUltrasoundFrequencyWidget& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
  this->ParameterName = "FrequencyMhz";
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundFrequencyWidgetPrivate::~qSlicerUltrasoundFrequencyWidgetPrivate()
{
  Q_Q(qSlicerUltrasoundFrequencyWidget);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundFrequencyWidgetPrivate::init()
{
  Q_Q(qSlicerUltrasoundFrequencyWidget);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->frequencySlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
  //QObject::connect(this->frequencySelector, SIGNAL(currentIndexChanged(int)), q, SLOT(setUltrasoundParameter()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundFrequencyWidgetPrivate::setupUi(QWidget *qSlicerUltrasoundFrequencyWidget)
{
  if (qSlicerUltrasoundFrequencyWidget->objectName().isEmpty())
  {
    qSlicerUltrasoundFrequencyWidget->setObjectName(QStringLiteral("qSlicerUltrasoundFrequencyWidget"));
  }

  qSlicerUltrasoundFrequencyWidget->resize(388, 44);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundFrequencyWidget->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundFrequencyWidget->setSizePolicy(sizePolicy);
  qSlicerUltrasoundFrequencyWidget->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundFrequencyWidget);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  label = new QLabel(qSlicerUltrasoundFrequencyWidget);
  label->setObjectName(QStringLiteral("label"));
  label->setText("Frequency (Mhz):");

  horizontalLayout->addWidget(label);

  frequencySlider = new ctkSliderWidget(qSlicerUltrasoundFrequencyWidget);
  frequencySlider->setObjectName(QStringLiteral("frequencySlider"));
  frequencySlider->setSingleStep(0.01);
  frequencySlider->setPageStep(0.1);
  frequencySlider->setMinimum(2.0);
  //frequencySlider->setMaximum(20.0);
  frequencySlider->setMaximum(5.0);
  frequencySlider->setTracking(false);
  horizontalLayout->addWidget(frequencySlider);

  //frequencySelector = new QComboBox(qSlicerUltrasoundFrequencyWidget);
  //frequencySelector->addItem("2500000");
  //frequencySelector->addItem("3300000");
  //frequencySelector->addItem("3333333");
  //frequencySelector->addItem("4000000");
  //frequencySelector->addItem("5000000");
  
  QMetaObject::connectSlotsByName(qSlicerUltrasoundFrequencyWidget);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundFrequencyWidget methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundFrequencyWidget::qSlicerUltrasoundFrequencyWidget(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundFrequencyWidgetPrivate(*this))
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  d->init();
  this->qvtkConnect(d->CmdSetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(setUltrasoundParameterCompleted));
  this->qvtkConnect(d->CmdGetParameter, vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this, SLOT(getUltrasoundParameterCompleted));

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundFrequencyWidget::~qSlicerUltrasoundFrequencyWidget()
{
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundFrequencyWidget::getFrequencyMhz()
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  return d->frequencySlider->value(); 
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundFrequencyWidget::setFrequencyMhz(double frequency)
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  d->frequencySlider->setValue(frequency);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundFrequencyWidget::onConnected()
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  d->frequencySlider->setEnabled(true);
  //d->frequencySelector->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundFrequencyWidget::onDisconnected()
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  d->frequencySlider->setDisabled(true);
  //d->frequencySelector->setDisabled(true);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundFrequencyWidget::getParameterValue()
{
  Q_D(qSlicerUltrasoundFrequencyWidget);
  return vtkVariant(this->getFrequencyMhz()).ToString();
  //return d->frequencySelector->currentText().toStdString();
}
