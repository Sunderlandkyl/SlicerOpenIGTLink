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

#ifndef __qSlicerUltrasoundDepthWidget_h
#define __qSlicerUltrasoundDepthWidget_h

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundRemoteControlModuleWidgetsExport.h"
#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include <QWidget>

class qSlicerUltrasoundDepthWidgetPrivate;

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerUltrasoundDepthWidget
  : public qSlicerAbstractUltrasoundParameterWidget
{
  Q_OBJECT

public:
  typedef qSlicerUltrasoundDepthWidget Superclass;
  /// Constructor
  qSlicerUltrasoundDepthWidget(QWidget* parent = 0);
  qSlicerUltrasoundDepthWidget(QWidget* parent, qSlicerUltrasoundDepthWidgetPrivate &d);

  /// Destructor
  virtual ~qSlicerUltrasoundDepthWidget();

  double getDepthMM();
  void setDepthMM(double depth);

  virtual void onConnected();
  virtual void onDisconnected();

  virtual void setParameter();
  virtual void updateParameter();
  virtual void onUpdate();

private:
  Q_DECLARE_PRIVATE(qSlicerUltrasoundDepthWidget);
  Q_DISABLE_COPY(qSlicerUltrasoundDepthWidget);
};

#endif
