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

#ifndef __qSlicerUltrasoundDynamicRangeWidget_h
#define __qSlicerUltrasoundDynamicRangeWidget_h

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundRemoteControlModuleWidgetsExport.h"

#include "qSlicerAbstractUltrasoundParameterWidget.h"

class qSlicerUltrasoundDynamicRangeWidgetPrivate;
class vtkMRMLIGTLConnectorNode; 

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerUltrasoundDynamicRangeWidget : public qSlicerAbstractUltrasoundParameterWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractUltrasoundParameterWidget Superclass;
  explicit qSlicerUltrasoundDynamicRangeWidget(QWidget *parent = 0);
  virtual ~qSlicerUltrasoundDynamicRangeWidget();

public slots:
  double getDynamicRangeDb();
  void setDynamicRangeDb(double dynamicRange);

  virtual void onConnected();
  virtual void onDisconnected();

protected:

  virtual std::string getParameterValue();

private:
  Q_DECLARE_PRIVATE(qSlicerUltrasoundDynamicRangeWidget);
  Q_DISABLE_COPY(qSlicerUltrasoundDynamicRangeWidget);
};

#endif
