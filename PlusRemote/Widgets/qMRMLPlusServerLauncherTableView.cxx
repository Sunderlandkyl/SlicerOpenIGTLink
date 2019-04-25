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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

// PlusRemote includes
#include "qMRMLPlusServerLauncherTableView.h"
#include "ui_qMRMLPlusServerLauncherTableView.h"

#include "vtkMRMLPlusServerLauncherNode.h"
#include "vtkMRMLPlusServerNode.h"
#include <vtkMRMLScene.h>

#include "vtkSlicerPlusRemoteLogic.h"

#include "vtkMRMLTextNode.h"

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>
#include <QKeyEvent>
#include <QStringList>
#include <QPushButton>

// SlicerQt includes
#include "qSlicerApplication.h"

enum ServerColumns
{
  Status,
  Name, 
  LogLevel,
  Description,
  LastColumn
};

enum
{
  ItemTypeServerNodeId = QTableWidgetItem::UserType,
};

//-----------------------------------------------------------------------------
class qMRMLPlusServerLauncherTableViewPrivate : public Ui_qMRMLPlusServerLauncherTableView
{
  Q_DECLARE_PUBLIC(qMRMLPlusServerLauncherTableView);

protected:
  qMRMLPlusServerLauncherTableView* const q_ptr;
public:
  qMRMLPlusServerLauncherTableViewPrivate(qMRMLPlusServerLauncherTableView& object);
  void init();

public:
  /// RT plan MRML node containing shown beams
  vtkWeakPointer<vtkMRMLPlusServerLauncherNode> LauncherNode;

private:
  QStringList ColumnLabels;
};

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableViewPrivate::qMRMLPlusServerLauncherTableViewPrivate(qMRMLPlusServerLauncherTableView& object)
  : q_ptr(&object)
{
  this->LauncherNode = nullptr;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableViewPrivate::init()
{
  Q_Q(qMRMLPlusServerLauncherTableView);
  this->setupUi(q);

  // Set table header properties
  for (int i = 0; i < LastColumn; ++i)
  {
    this->ColumnLabels << "";
  }
  this->ColumnLabels[ServerColumns::Name] = "Name";
  this->ColumnLabels[ServerColumns::LogLevel] = "Log level";
  this->ColumnLabels[ServerColumns::Description] = "Description";
  this->ColumnLabels[ServerColumns::Status] = "Status";
  this->PlusServerLauncherTable->setHorizontalHeaderLabels(this->ColumnLabels);
  this->PlusServerLauncherTable->setColumnCount(this->ColumnLabels.size());

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  this->PlusServerLauncherTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
  this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
  this->PlusServerLauncherTable->horizontalHeader()->setStretchLastSection(1);

  // Select rows
  this->PlusServerLauncherTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Make connections
  QObject::connect(this->PlusServerLauncherTable, SIGNAL(itemSelectionChanged()),
    q, SLOT(onTableSelectionChanged()));
  QObject::connect(this->AddServerButton, SIGNAL(clicked()), q, SLOT(onAddServer()));
  QObject::connect(this->RemoveServerButton, SIGNAL(clicked()), q, SLOT(onRemoveServer()));
  QObject::connect(this->ConfigFileComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(updateMRMLFromWidget()));
  QObject::connect(this->StartStopButton, SIGNAL(clicked()), q, SLOT(onStartStopServer()));

  this->PlusServerLauncherTable->installEventFilter(q);
}

//-----------------------------------------------------------------------------
// qMRMLPlusServerLauncherTableView methods

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableView::qMRMLPlusServerLauncherTableView(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLPlusServerLauncherTableViewPrivate(*this))
{
  Q_D(qMRMLPlusServerLauncherTableView);
  d->init();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableView::~qMRMLPlusServerLauncherTableView() = default;

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::setLauncherNode(vtkMRMLNode* node)
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerLauncherNode* launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(node);

  // Connect modified events to population of the table
  qvtkReconnect(d->LauncherNode, launcherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerAddedEvent, this, SLOT(onServerAdded(vtkObject*, void*)));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerRemovedEvent, this, SLOT(onServerRemoved(vtkObject*, void*)));

  if (d->LauncherNode)
  {
    std::vector<vtkMRMLPlusServerNode*> servers = d->LauncherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    }
  }

  if (launcherNode)
  {
    // Connect modified events of contained beam nodes to update table
    std::vector<vtkMRMLPlusServerNode*> servers = launcherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    }
  }

  d->LauncherNode = launcherNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLPlusServerLauncherTableView::launcherNode()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  return d->LauncherNode;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateWidgetFromMRML()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  d->PlusServerLauncherTable->setEnabled(d->LauncherNode);
  d->AddServerButton->setEnabled(d->LauncherNode);
  d->RemoveServerButton->setEnabled(d->LauncherNode);
  d->ConfigFileComboBox->setEnabled(d->LauncherNode);
  d->StartStopButton->setEnabled(d->LauncherNode);

  if (!d->LauncherNode)
  {
    return;
  }

  vtkMRMLPlusServerNode* selectedServerNode = this->getSelectedServer();

  // Block signals so that onBeamTableItemChanged function is not called when populating
  bool wasBlocking = d->PlusServerLauncherTable->blockSignals(true);
  d->PlusServerLauncherTable->clearContents();

  std::vector<vtkMRMLPlusServerNode*> serverNodes = d->LauncherNode->GetServerNodes();
  int numberOfNodes = serverNodes.size();
  d->PlusServerLauncherTable->setRowCount(numberOfNodes);
  for (int i = 0; i < numberOfNodes; ++i)
  {
    vtkMRMLPlusServerNode* serverNode = serverNodes[i];
    for (int j = 0; j < ServerColumns::LastColumn; ++j)
    {
      QTableWidgetItem* item = new QTableWidgetItem();
      item->setData(ItemTypeServerNodeId, serverNode->GetID());
      d->PlusServerLauncherTable->setItem(i, j, item);
    }

    QString logLevel;
    switch (serverNode->GetLogLevel())
    {
    case vtkMRMLPlusServerNode::LOG_DEBUG:
      logLevel = "Debug";
      break;
    case vtkMRMLPlusServerNode::LOG_ERROR:
      logLevel = "Error";
      break;
    case vtkMRMLPlusServerNode::LOG_INFO:
      logLevel = "Info";
      break;
    case vtkMRMLPlusServerNode::LOG_TRACE:
      logLevel = "Trace";
      break;
    case vtkMRMLPlusServerNode::LOG_WARNING:
      logLevel = "Warning";
      break;
    }

    QString status;
    switch (serverNode->GetState())
    {
    case vtkMRMLPlusServerNode::ServerStatus::On:
      status = "On";
      break;
    case vtkMRMLPlusServerNode::ServerStatus::Off:
      status = "Off";
      break;
    case vtkMRMLPlusServerNode::ServerStatus::Starting:
      status = "Starting";
      break;
    case vtkMRMLPlusServerNode::ServerStatus::Stopping:
      status = "Stopping";
      break;
    }

    std::string name = serverNode->GetDeviceSetName();
    std::string  decription = serverNode->GetDeviceSetDescription();
    d->PlusServerLauncherTable->item(i, ServerColumns::Name)->setText(name.c_str());
    d->PlusServerLauncherTable->item(i, ServerColumns::LogLevel)->setText(logLevel);
    d->PlusServerLauncherTable->item(i, ServerColumns::Description)->setText(decription.c_str());
    d->PlusServerLauncherTable->item(i, ServerColumns::Status)->setText(status);
  }

  if (selectedServerNode)
  {
    for (int i = 0; i < numberOfNodes; ++i)
    {
      QTableWidgetItem* item = d->PlusServerLauncherTable->item(i, 0);
      if (item && item->data(ItemTypeServerNodeId) == selectedServerNode->GetID())
      {
        d->PlusServerLauncherTable->selectRow(i);
        break;
      }
    }
  }

  // Unblock signals
  d->PlusServerLauncherTable->blockSignals(wasBlocking);

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  vtkMRMLTextNode* configFileNode = nullptr;
  if (serverNode)
  {
    configFileNode = serverNode->GetConfigNode();
    if (serverNode->GetDesiredState() == vtkMRMLPlusServerNode::ServerStatus::On)
    {
      d->StartStopButton->setText("Stop server");
    }
    else
    {
      d->StartStopButton->setText("Launch server");
    }
  }

  d->ConfigFileComboBox->setEnabled(serverNode);
  if (configFileNode)
  {
    wasBlocking = d->ConfigFileComboBox->blockSignals(true);
    d->ConfigFileComboBox->setCurrentNodeID(configFileNode->GetID());
    d->ConfigFileComboBox->blockSignals(wasBlocking);
  }

  d->StartStopButton->setEnabled(serverNode && configFileNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateMRMLFromWidget()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  if (!serverNode)
  {
    return;
  }
  int wasModifying = serverNode->StartModify();

  vtkMRMLTextNode* configFileNode = vtkMRMLTextNode::SafeDownCast(d->ConfigFileComboBox->currentNode());
  serverNode->SetAndObserveConfigNode(configFileNode);

  serverNode->EndModify(wasModifying);
}

//-----------------------------------------------------------------------------
vtkMRMLPlusServerNode* qMRMLPlusServerLauncherTableView::getSelectedServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  int row = d->PlusServerLauncherTable->currentRow();
  if (row < 0)
  {
    return nullptr;
  }

  QTableWidgetItem* item = d->PlusServerLauncherTable->currentItem();
  if (!item)
  {
    return nullptr;
  }

  QVariant itemData = item->data(ItemTypeServerNodeId);
  std::string serverNodeId = itemData.toString().toStdString();
  return vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->GetNodeByID(serverNodeId));
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onTableSelectionChanged()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();

  d->ConfigFileComboBox->setEnabled(serverNode);

  vtkMRMLTextNode* configFileNode = nullptr;
  if (serverNode)
  {
    configFileNode = serverNode->GetConfigNode();
  }
  d->StartStopButton->setEnabled(serverNode && configFileNode);
  d->ConfigFileComboBox->setCurrentNodeID(configFileNode ? configFileNode->GetID() : "");
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onAddServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerNode", "ServerTest"));
  d->LauncherNode->AddAndObserveServerNode(serverNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onRemoveServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  d->LauncherNode->RemoveServerNode(serverNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onStartStopServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);
  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  if (!serverNode)
  {
    return;
  }

  if (serverNode->GetDesiredState() == vtkMRMLPlusServerNode::ServerStatus::On)
  {
    serverNode->StopServer();
    d->StartStopButton->setText("Launch server");
  }
  else
  {
    serverNode->StartServer();
    d->StartStopButton->setText("Stop server");
  }
}

//------------------------------------------------------------------------------
bool qMRMLPlusServerLauncherTableView::eventFilter(QObject* target, QEvent* event)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  if (target == d->PlusServerLauncherTable)
  {
    // Prevent giving the focus to the previous/next widget if arrow keys are used
    // at the edge of the table (without this: if the current cell is in the top
    // row and user press the Up key, the focus goes from the table to the previous
    // widget in the tab order)
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      QAbstractItemModel* model = d->PlusServerLauncherTable->model();
      QModelIndex currentIndex = d->PlusServerLauncherTable->currentIndex();

      if (model && (
        (keyEvent->key() == Qt::Key_Left && currentIndex.column() == 0)
        || (keyEvent->key() == Qt::Key_Up && currentIndex.row() == 0)
        || (keyEvent->key() == Qt::Key_Right && currentIndex.column() == model->columnCount() - 1)
        || (keyEvent->key() == Qt::Key_Down && currentIndex.row() == model->rowCount() - 1)))
      {
        return true;
      }
    }
  }
  return this->QWidget::eventFilter(target, event);
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onServerAdded(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  Q_UNUSED(caller);

  vtkMRMLPlusServerNode* serverNode = reinterpret_cast<vtkMRMLPlusServerNode*>(callData);
  if (!serverNode)
  {
    return;
  }
  qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
}

//------------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::onServerRemoved(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);
  Q_UNUSED(caller);

  vtkMRMLPlusServerNode* serverNode = reinterpret_cast<vtkMRMLPlusServerNode*>(callData);
  if (!serverNode)
  {
    return;
  }
  qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
}
