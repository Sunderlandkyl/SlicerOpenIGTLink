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
#include "qSlicerUltrasoundDoubleParameterSlider.h"
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkMRMLIGTLConnectorNode.h"

// VTK includes
#include <vtkVariant.h>
#include <vtkXMLUtilities.h>

// Qt includes
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

// ctk includes
#include <ctkDoubleSpinBox.h>
#include <ctkSliderWidget.h>

//-----------------------------------------------------------------------------
class qSlicerUltrasoundDoubleParameterSliderPrivate : qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerUltrasoundDoubleParameterSlider);

  qSlicerUltrasoundDoubleParameterSliderPrivate(qSlicerUltrasoundDoubleParameterSlider& p);
  ~qSlicerUltrasoundDoubleParameterSliderPrivate();

public:
  virtual void init();
  virtual void setupUi(QWidget *qSlicerUltrasoundDoubleParameterSlider);

protected:
  qSlicerUltrasoundDoubleParameterSlider* const q_ptr;

public:
  QHBoxLayout *horizontalLayout;
  ctkSliderWidget *expectedValueSlider;
  ctkDoubleSpinBox *actualValueSpinBox;
};

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSliderPrivate::qSlicerUltrasoundDoubleParameterSliderPrivate(qSlicerUltrasoundDoubleParameterSlider& object)
  : qSlicerAbstractUltrasoundParameterWidgetPrivate(&object)
  , q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSliderPrivate::~qSlicerUltrasoundDoubleParameterSliderPrivate()
{
  Q_Q(qSlicerUltrasoundDoubleParameterSlider);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSliderPrivate::init()
{
  Q_Q(qSlicerUltrasoundDoubleParameterSlider);
  qSlicerAbstractUltrasoundParameterWidgetPrivate::init();

  this->setupUi(q);
  QObject::connect(this->expectedValueSlider, SIGNAL(valueChanged(double)), q, SLOT(setUltrasoundParameter()));
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSliderPrivate::setupUi(QWidget *qSlicerUltrasoundDoubleParameterSlider)
{
  if (qSlicerUltrasoundDoubleParameterSlider->objectName().isEmpty())
  {
    qSlicerUltrasoundDoubleParameterSlider->setObjectName(QStringLiteral("qSlicerUltrasoundDoubleParameterSlider"));
  }

  qSlicerUltrasoundDoubleParameterSlider->resize(388, 44);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(qSlicerUltrasoundDoubleParameterSlider->sizePolicy().hasHeightForWidth());
  qSlicerUltrasoundDoubleParameterSlider->setSizePolicy(sizePolicy);
  qSlicerUltrasoundDoubleParameterSlider->setMinimumSize(QSize(10, 10));
  horizontalLayout = new QHBoxLayout(qSlicerUltrasoundDoubleParameterSlider);
  horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
  horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
  
  this->expectedValueSlider = new ctkSliderWidget(qSlicerUltrasoundDoubleParameterSlider);
  this->expectedValueSlider->setObjectName(QStringLiteral("slider"));
  this->expectedValueSlider->setTracking(false);
  horizontalLayout->addWidget(this->expectedValueSlider);

  QLabel* actualValueLabel = new QLabel(qSlicerUltrasoundDoubleParameterSlider);
  actualValueLabel->setObjectName(QStringLiteral("actualValueLabel"));
  actualValueLabel->setText("   Current value:");
  horizontalLayout->addWidget(actualValueLabel);
  
  this->actualValueSpinBox = new ctkDoubleSpinBox(qSlicerUltrasoundDoubleParameterSlider);
  this->actualValueSpinBox->spinBox()->setReadOnly(true);
  this->actualValueSpinBox->spinBox()->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
  this->actualValueSpinBox->lineEdit()->setReadOnly(true);
  horizontalLayout->addWidget(this->actualValueSpinBox);
}

//-----------------------------------------------------------------------------
// qSlicerUltrasoundDoubleParameterSlider methods

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSlider::qSlicerUltrasoundDoubleParameterSlider(QWidget* _parent)
  : qSlicerAbstractUltrasoundParameterWidget(new qSlicerUltrasoundDoubleParameterSliderPrivate(*this))
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->init();

  this->onConnectionChanged();
}

//-----------------------------------------------------------------------------
qSlicerUltrasoundDoubleParameterSlider::~qSlicerUltrasoundDoubleParameterSlider()
{
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::onConnected()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->expectedValueSlider->setEnabled(true);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::onDisconnected()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->expectedValueSlider->setDisabled(true);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundDoubleParameterSlider::getExpectedParameterValue()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  return vtkVariant(d->expectedValueSlider->value()).ToString();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setExpectedParameterValue(std::string expectedParameterString)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);

  bool wasBlocking = d->expectedValueSlider->blockSignals(true);
  bool valid = false;
  double value = vtkVariant(expectedParameterString).ToDouble(&valid);
  if (valid)
  {
    d->expectedValueSlider->setValue(value);
  }
  d->expectedValueSlider->blockSignals(wasBlocking);
}

//-----------------------------------------------------------------------------
std::string qSlicerUltrasoundDoubleParameterSlider::getActualParameterValue()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  return vtkVariant(d->actualValueSpinBox->value()).ToString();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setActualParameterValue(std::string actualParameterString)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);

  bool wasBlocking = d->actualValueSpinBox->blockSignals(true);
  bool valid = false;
  double value = vtkVariant(actualParameterString).ToDouble(&valid);
  if (valid)
  {
    d->actualValueSpinBox->setValue(value);
  }
  d->actualValueSpinBox->blockSignals(wasBlocking);
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::getUltrasoundParameterCompleted()
{
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setUltrasoundParameterCompleted()
{
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::minimum()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  return d->expectedValueSlider->minimum();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setMinimum(double minimum)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->expectedValueSlider->setMinimum(minimum);
}

//-----------------------------------------------------------------------------
double qSlicerUltrasoundDoubleParameterSlider::maximum()
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  return d->expectedValueSlider->maximum();
}

//-----------------------------------------------------------------------------
void qSlicerUltrasoundDoubleParameterSlider::setMaximum(double maximum)
{
  Q_D(qSlicerUltrasoundDoubleParameterSlider);
  d->expectedValueSlider->setMaximum(maximum);
}