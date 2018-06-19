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

#include "qSlicerAbstractUltrasoundParameterWidget.h"
#include "qSlicerAbstractUltrasoundParameterWidget_p.h"

#include "vtkSlicerOpenIGTLinkCommand.h"

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidgetPrivate::qSlicerAbstractUltrasoundParameterWidgetPrivate(qSlicerAbstractUltrasoundParameterWidget *q)
  : ParameterName("")
  , ParameterValue("")
  , CmdSetParameter(vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New())
  , CmdUpdateParameter(vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New())
  , ConnectorNode(NULL)
  , q_ptr(q)
{
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(QWidget* parentWidget)
  : QWidget(parentWidget)
  , d_ptr(new qSlicerAbstractUltrasoundParameterWidgetPrivate(this))
{
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::qSlicerAbstractUltrasoundParameterWidget(QWidget* parentWidget, qSlicerAbstractUltrasoundParameterWidgetPrivate &d)
  : QWidget(parentWidget)
  , d_ptr(&d)
{
}

//-----------------------------------------------------------------------------
qSlicerAbstractUltrasoundParameterWidget::~qSlicerAbstractUltrasoundParameterWidget()
{
}

//-----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* qSlicerAbstractUltrasoundParameterWidget::getConnectorNode()
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  return d->ConnectorNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerAbstractUltrasoundParameterWidget::setConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode)
{
  Q_D(qSlicerAbstractUltrasoundParameterWidget);
  d->ConnectorNode = connectorNode;
}