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

class qSlicerAbstractUltrasoundParameterWidgetPrivate;

/// \ingroup Slicer_QtModules_UltrasoundRemoteControl
class Q_SLICER_MODULE_ULTRASOUNDREMOTECONTROL_WIDGETS_EXPORT qSlicerAbstractUltrasoundParameterWidget : public QWidget
{
  Q_OBJECT

public:
  typedef QWidget Superclass;
  /// Constructor
  qSlicerAbstractUltrasoundParameterWidget(QWidget* parent = 0);
  qSlicerAbstractUltrasoundParameterWidget(QWidget* parent, qSlicerAbstractUltrasoundParameterWidgetPrivate &d);

  /// Destructor
  virtual ~qSlicerAbstractUltrasoundParameterWidget();

public slots:

  vtkMRMLIGTLConnectorNode* getConnectorNode();
  void setConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode);

  virtual void onConnected() = 0;
  virtual void onDisconnected() = 0;

  virtual void setParameter() = 0;
  virtual void updateParameter() = 0;
  virtual void onUpdate() = 0;

protected:

  qSlicerAbstractUltrasoundParameterWidget(qSlicerAbstractUltrasoundParameterWidgetPrivate &dd, QWidget* parent = nullptr);

  QScopedPointer<qSlicerAbstractUltrasoundParameterWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerAbstractUltrasoundParameterWidget);
  Q_DISABLE_COPY(qSlicerAbstractUltrasoundParameterWidget);
};


#endif
