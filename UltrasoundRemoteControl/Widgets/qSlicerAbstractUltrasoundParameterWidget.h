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

#ifndef __qSlicerAbstractUltrasoundParameterWidget_h
#define __qSlicerAbstractUltrasoundParameterWidget_h

// UltrasoundRemoteControl includes
#include "qSlicerUltrasoundRemoteControlModuleWidgetsExport.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QWidget>

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>

#include <qMRMLWidget.h>

// CTK includes
#include <ctkVTKObject.h>

class qSlicerAbstractUltrasoundParameterWidgetPrivate;

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerAbstractUltrasoundParameterWidget : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

  Q_PROPERTY(std::string  parameterName READ parameterName WRITE setParameterName)

public:
  typedef qMRMLWidget Superclass;
  qSlicerAbstractUltrasoundParameterWidget(QWidget* parent = 0);
  virtual ~qSlicerAbstractUltrasoundParameterWidget();

protected:
  qSlicerAbstractUltrasoundParameterWidget(qSlicerAbstractUltrasoundParameterWidgetPrivate* d);

public slots:

  const char* parameterName();
  void setParameterName(const char* name);

  vtkMRMLIGTLConnectorNode* getConnectorNode();
  void setConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode);

  virtual void onConnectionChanged();
  virtual void onConnected() {};
  virtual void onDisconnected() {};

  virtual void setUltrasoundParameter();
  virtual void getUltrasoundParameter();

  void setDeviceID(std::string deviceID);
  std::string getDeviceID();

protected:

  virtual std::string getExpectedParameterValue() { return ""; };
  virtual void setExpectedParameterValue(std::string value) {};

  virtual std::string getActualParameterValue() { return ""; };
  virtual void setActualParameterValue(std::string value) {};

  QScopedPointer<qSlicerAbstractUltrasoundParameterWidgetPrivate> d_ptr;

protected slots:

  virtual void setUltrasoundParameterCompleted();
  virtual void getUltrasoundParameterCompleted();

private:
  Q_DECLARE_PRIVATE(qSlicerAbstractUltrasoundParameterWidget);
  Q_DISABLE_COPY(qSlicerAbstractUltrasoundParameterWidget);
};


#endif
