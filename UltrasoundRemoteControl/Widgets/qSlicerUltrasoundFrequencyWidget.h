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

#ifndef __qSlicerUltrasoundFrequencyWidget_h
#define __qSlicerUltrasoundFrequencyWidget_h

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundRemoteControlModuleWidgetsExport.h"

#include "qSlicerAbstractUltrasoundParameterWidget.h"

class qSlicerUltrasoundFrequencyWidgetPrivate;
class vtkMRMLIGTLConnectorNode; 

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerUltrasoundFrequencyWidget : public qSlicerAbstractUltrasoundParameterWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractUltrasoundParameterWidget Superclass;
  explicit qSlicerUltrasoundFrequencyWidget(QWidget *parent = 0);
  virtual ~qSlicerUltrasoundFrequencyWidget();

public slots:
  double getFrequencyMhz();
  void setFrequencyMhz(double frequency);

  virtual void onConnected();
  virtual void onDisconnected();

protected:

  virtual std::string getParameterValue();

private:
  Q_DECLARE_PRIVATE(qSlicerUltrasoundFrequencyWidget);
  Q_DISABLE_COPY(qSlicerUltrasoundFrequencyWidget);
};

#endif
