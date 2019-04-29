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
  ID,
  Name,
  Control,
  State,
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

  QPixmap IconLaunch;
  QPixmap IconStop;
  QPixmap IconOn;
  QPixmap IconOff;
  QPixmap IconStarting;
  QPixmap IconStopping;

private:
  QStringList ColumnLabels;
};

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableViewPrivate::qMRMLPlusServerLauncherTableViewPrivate(qMRMLPlusServerLauncherTableView& object)
  : q_ptr(&object)
  , IconLaunch(":/Icons/PlusLauncherRemoteDisconnected.png")
  , IconStop(":/Icons/PlusLauncherRemoteDisconnected.png")
  , IconOn(":/Icons/PlusLauncherRemoteDisconnected.png")
  , IconOff(":/Icons/PlusLauncherRemoteDisconnected.png")
  , IconStarting(":/Icons/PlusLauncherRemoteDisconnected.png")
  , IconStopping(":/Icons/PlusLauncherRemoteDisconnected.png")
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
  this->ColumnLabels[ServerColumns::ID] = "ID";
  this->ColumnLabels[ServerColumns::Name] = "Name";
  this->ColumnLabels[ServerColumns::Control] = "Control";
  this->ColumnLabels[ServerColumns::State] = "State";
  this->PlusServerLauncherTable->setHorizontalHeaderLabels(this->ColumnLabels);
  this->PlusServerLauncherTable->setColumnCount(this->ColumnLabels.size());
  for (int i = 0; i < this->ColumnLabels.size(); ++i)
  {
    if (i == ServerColumns::Name)
    {
      this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    else
    {
      this->PlusServerLauncherTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
    }
  }

  // Select rows
  this->PlusServerLauncherTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Make connections
  QObject::connect(this->PlusServerLauncherTable, SIGNAL(itemSelectionChanged()),
    q, SLOT(updateSelectedServerPanel()));
  QObject::connect(this->AddServerButton, SIGNAL(clicked()), q, SLOT(onAddServer()));
  QObject::connect(this->RemoveServerButton, SIGNAL(clicked()), q, SLOT(onRemoveServer()));
  QObject::connect(this->ConfigFileComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(updateServerMRMLFromWidget()));

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
  this->updateWidgetFromLauncherMRML();
}

//-----------------------------------------------------------------------------
qMRMLPlusServerLauncherTableView::~qMRMLPlusServerLauncherTableView() = default;

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::setLauncherNode(vtkMRMLNode* node)
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerLauncherNode* launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(node);

  // Connect modified events to population of the table
  qvtkReconnect(d->LauncherNode, launcherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromLauncherMRML()));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerAddedEvent, this, SLOT(onServerAdded(vtkObject*, void*)));
  qvtkReconnect(d->LauncherNode, launcherNode, vtkMRMLPlusServerLauncherNode::ServerRemovedEvent, this, SLOT(onServerRemoved(vtkObject*, void*)));

  if (d->LauncherNode)
  {
    std::vector<vtkMRMLPlusServerNode*> servers = d->LauncherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
    }
  }

  if (launcherNode)
  {
    // Connect modified events of contained beam nodes to update table
    std::vector<vtkMRMLPlusServerNode*> servers = launcherNode->GetServerNodes();
    for (std::vector<vtkMRMLPlusServerNode*>::iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt)
    {
      vtkMRMLPlusServerNode* serverNode = (*serverIt);
      serverNode->UpdateConfigFileInfo();
      qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
    }
  }

  d->LauncherNode = launcherNode;
  this->updateWidgetFromLauncherMRML();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLPlusServerLauncherTableView::launcherNode()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  return d->LauncherNode;
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateWidgetFromLauncherMRML()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  d->PlusServerLauncherTable->setEnabled(d->LauncherNode);
  d->AddServerButton->setEnabled(d->LauncherNode);
  d->RemoveServerButton->setEnabled(d->LauncherNode);
  d->ConfigFileComboBox->setEnabled(d->LauncherNode);

  if (!d->LauncherNode)
  {
    return;
  }

  // Block signals so that onBeamTableItemChanged function is not called when populating
  bool tableWasBlocking = d->PlusServerLauncherTable->blockSignals(true);
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
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      d->PlusServerLauncherTable->setItem(i, j, item);
      if (j == ServerColumns::Control)
      {
        QPushButton* startStopButton = new QPushButton(this);
        startStopButton->setIcon(d->IconLaunch);
        d->PlusServerLauncherTable->setCellWidget(i, j, startStopButton);
        QObject::connect(startStopButton, SIGNAL(clicked()), this, SLOT(onStartStopServer()));
      }
    }

    this->updateWidgetFromServerMRML(serverNode);
  }

  vtkMRMLPlusServerNode* selectedServerNode = this->getSelectedServer();
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
  d->PlusServerLauncherTable->blockSignals(tableWasBlocking);

  this->updateSelectedServerPanel();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateWidgetFromServerMRML(vtkObject* caller, void* callData)
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(caller);
  if (serverNode)
  {
    std::string name = serverNode->GetDeviceSetName();
    std::string  decription = serverNode->GetDeviceSetDescription();
    int logLevel = serverNode->GetLogLevel();
    QString state = serverNode->GetStateAsString(serverNode->GetState());

    int row = this->getRowForServer(serverNode);
    d->PlusServerLauncherTable->item(row, ServerColumns::Name)->setText(name.c_str());
    d->PlusServerLauncherTable->item(row, ServerColumns::State)->setIcon(d->IconOn);
  }

  this->updateSelectedServerPanel();
}

//-----------------------------------------------------------------------------
void qMRMLPlusServerLauncherTableView::updateServerMRMLFromWidget()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  if (!d->LauncherNode)
  {
    return;
  }

  vtkMRMLPlusServerNode* selectedServerNode = this->getSelectedServer();
  for (vtkMRMLPlusServerNode* serverNode : d->LauncherNode->GetServerNodes())
  {
    int wasModifying = serverNode->StartModify();
    int logLevel = d->LogLevelComboBox->currentData().toInt();
    serverNode->SetLogLevel(logLevel);
    if (selectedServerNode && selectedServerNode == serverNode)
    {
      vtkMRMLTextNode* configFileNode = vtkMRMLTextNode::SafeDownCast(d->ConfigFileComboBox->currentNode());
      selectedServerNode->SetAndObserveConfigNode(configFileNode);
    }
    serverNode->EndModify(wasModifying);
  }
}

//-----------------------------------------------------------------------------
int qMRMLPlusServerLauncherTableView::getRowForServer(vtkMRMLPlusServerNode* serverNode) const
{
  Q_D(const qMRMLPlusServerLauncherTableView);
  if (!serverNode)
  {
    return -1;
  }

  for (int i = 0; i < d->PlusServerLauncherTable->rowCount(); ++i)
  {
    QVariant rowData = d->PlusServerLauncherTable->item(i, 0)->data(ItemTypeServerNodeId);
    QString rowString = rowData.toString();
    if (rowData == serverNode->GetID())
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
vtkMRMLPlusServerNode* qMRMLPlusServerLauncherTableView::getSelectedServer()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  int row = -1;

  QList<QTableWidgetSelectionRange> ranges = d->PlusServerLauncherTable->selectedRanges();
  for (QTableWidgetSelectionRange range : ranges)
  {
    row = std::max(range.topRow(), row);
  }

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
void qMRMLPlusServerLauncherTableView::updateSelectedServerPanel()
{
  Q_D(qMRMLPlusServerLauncherTableView);

  vtkMRMLPlusServerNode* serverNode = this->getSelectedServer();
  d->ConfigFileComboBox->setEnabled(serverNode);


  vtkMRMLTextNode* configFileNode = nullptr;
  QString configFileNodeID;
  if (serverNode)
  {
    configFileNode = serverNode->GetConfigNode();
    if (configFileNode)
    {
      configFileNodeID = configFileNode->GetID();
    }

    d->ConfigFileComboBox->setEnabled(serverNode->GetDesiredState() == vtkMRMLPlusServerNode::State::Off &&
      serverNode->GetState() == vtkMRMLPlusServerNode::State::Off);
  }

  bool configComboBoxWasBlocking = d->ConfigFileComboBox->blockSignals(true);
  d->ConfigFileComboBox->setCurrentNodeID(configFileNodeID);
  d->ConfigFileComboBox->blockSignals(configComboBoxWasBlocking);
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

  vtkMRMLPlusServerNode* serverNode = vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerNode"));
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
  this->mrmlScene()->RemoveNode(serverNode);
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
  qvtkConnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
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
  qvtkDisconnect(serverNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromServerMRML(vtkObject*, void*)));
}
