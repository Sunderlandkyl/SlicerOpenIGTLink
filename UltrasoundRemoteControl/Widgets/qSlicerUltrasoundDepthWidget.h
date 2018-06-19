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

#include <qMRMLWidget.h>

// CTK includes
#include <ctkVTKObject.h>

class qSlicerUltrasoundDepthWidgetPrivate;
class vtkMRMLIGTLConnectorNode;

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerUltrasoundDepthWidget : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerUltrasoundDepthWidget Superclass;
  explicit qSlicerUltrasoundDepthWidget(QWidget *parent = 0);
  virtual ~qSlicerUltrasoundDepthWidget();


public slots:
  double getDepthMM();
  void setDepthMM(double depth);

  virtual void onConnectionChanged();
  virtual void onConnected();
  virtual void onDisconnected();

  virtual void updateParameter();
  virtual void onUpdate();

  void setConnectorNode(vtkMRMLIGTLConnectorNode* node);
  vtkMRMLIGTLConnectorNode* getConnectorNode();

  void setDeviceID(std::string deviceID);
  std::string getDeviceID();

protected:

  QScopedPointer<qSlicerUltrasoundDepthWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerUltrasoundDepthWidget);
  Q_DISABLE_COPY(qSlicerUltrasoundDepthWidget);
};

#endif
