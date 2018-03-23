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

// PlusRemote includes
#include "qMRMLPlusLauncherRemoteWidget.h"
#include "ui_qMRMLPlusLauncherRemoteWidget.h"
#include "vtkMRMLPlusRemoteLauncherNode.h"

// QT includes
#include <QtGui>
#include <QDebug>
#include <qfiledialog.h>
#include <QIntValidator>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLAbstractLogic.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

// MRML includes
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include <vtkMRMLTextNode.h>
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkSlicerOpenIGTLinkCommand.h>

//-----------------------------------------------------------------------------
const char* CONFIG_FILE_NODE_ATTRIBUTE = "ConfigFile";

const int LOG_LEVEL_ERROR = 1;
const int LOG_LEVEL_WARNING = 2;
const int LOG_LEVEL_INFO = 3;
const int LOG_LEVEL_DEBUG = 4;
const int LOG_LEVEL_TRACE = 5;

const char* COLOR_NORMAL = "#000000";
const char* COLOR_WARNING = "#FF8000";
const char* COLOR_ERROR = "#D70000";

const char* PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID = "PSL_Remote";

//-----------------------------------------------------------------------------
class qMRMLPlusLauncherRemoteWidgetPrivate : public Ui_qMRMLPlusLauncherRemoteWidget
{
  Q_DECLARE_PUBLIC(qMRMLPlusLauncherRemoteWidget);

protected:
  qMRMLPlusLauncherRemoteWidget* const q_ptr;
public:
  qMRMLPlusLauncherRemoteWidgetPrivate(qMRMLPlusLauncherRemoteWidget& object);
  ~qMRMLPlusLauncherRemoteWidgetPrivate();
  void init();

  //TODO: look up QT style for member variables, etc.
  vtkSmartPointer<vtkCallbackCommand> startServerCallback;
  vtkSmartPointer<vtkCallbackCommand> stopServerCallback;
  vtkSmartPointer<vtkCallbackCommand> commandReceivedCallback;
  vtkSmartPointer<vtkCallbackCommand> onServerInfoReceivedCallback;

  vtkWeakPointer<vtkMRMLPlusRemoteLauncherNode> ParameterSetNode;
  vtkWeakPointer<vtkMRMLIGTLConnectorNode> LauncherConnectorNode;

  vtkSmartPointer<vtkMRMLTextNode> LogTextNode;

  QPixmap IconGreenDot;
  QPixmap IconGreyDot;
  QPixmap IconOrangeDot;
  QPixmap IconRedDot;

  int serverErrorLevel;
  int launcherErrorLevel;

};

//-----------------------------------------------------------------------------
qMRMLPlusLauncherRemoteWidgetPrivate::qMRMLPlusLauncherRemoteWidgetPrivate(qMRMLPlusLauncherRemoteWidget& object)
  : q_ptr(&object)
  , LauncherConnectorNode(NULL)
{
  this->IconGreenDot = QPixmap(":/Icons/PlusLauncherRemoteDotGreen.png");
  this->IconGreyDot = QPixmap(":/Icons/PlusLauncherRemoteDotGrey.png");
  this->IconOrangeDot = QPixmap(":/Icons/PlusLauncherRemoteDotOrange.png");
  this->IconRedDot = QPixmap(":/Icons/PlusLauncherRemoteDotRed.png");
}

//-----------------------------------------------------------------------------
qMRMLPlusLauncherRemoteWidgetPrivate::~qMRMLPlusLauncherRemoteWidgetPrivate()
{
  Q_Q(qMRMLPlusLauncherRemoteWidget);
}

void qMRMLPlusLauncherRemoteWidgetPrivate::init()
{
  Q_Q(qMRMLPlusLauncherRemoteWidget);
  this->setupUi(q);

  this->portLineEdit->setValidator(new QIntValidator(0, 65535, this->portLineEdit));
  this->configFileSelectorComboBox->addAttribute("vtkMRMLTextNode", CONFIG_FILE_NODE_ATTRIBUTE);

  QObject::connect(this->launcherConnectCheckBox, SIGNAL(toggled(bool)), q, SLOT(onConnectCheckBoxChanged(bool)));
  QObject::connect(this->loadConfigFileButton, SIGNAL(clicked()), q, SLOT(onLoadConfigFile()));
  QObject::connect(this->configFileSelectorComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(onConfigFileChanged(vtkMRMLNode*)));
  QObject::connect(this->startStopServerButton, SIGNAL(clicked()), q, SLOT(onStartStopButton()));
  QObject::connect(this->clearLogButton, SIGNAL(clicked()), q, SLOT(onClearLogButton()));

  this->logLevelComboBox->addItem("Error", LOG_LEVEL_ERROR);
  this->logLevelComboBox->addItem("Warning", LOG_LEVEL_WARNING);
  this->logLevelComboBox->addItem("Info", LOG_LEVEL_INFO);
  this->logLevelComboBox->addItem("Debug", LOG_LEVEL_DEBUG);
  int logInfoIndex = this->logLevelComboBox->findData(LOG_LEVEL_INFO);
  if (logInfoIndex >= 0)
  {
    this->logLevelComboBox->setCurrentIndex(logInfoIndex);
  }
  this->serverErrorLevel = LOG_LEVEL_INFO;
  this->launcherErrorLevel = LOG_LEVEL_INFO;

  //this->logLevelComboBox->addItem("Trace", LOG_LEVEL_TRACE);

  this->startServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->startServerCallback->SetCallback(qMRMLPlusLauncherRemoteWidget::onStartServerResponse);
  this->startServerCallback->SetClientData(this);

  this->stopServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->stopServerCallback->SetCallback(qMRMLPlusLauncherRemoteWidget::onStopServerResponse);
  this->stopServerCallback->SetClientData(this);

  this->commandReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->commandReceivedCallback->SetCallback(qMRMLPlusLauncherRemoteWidget::onCommandReceived);
  this->commandReceivedCallback->SetClientData(this);

  this->onServerInfoReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->onServerInfoReceivedCallback->SetCallback(qMRMLPlusLauncherRemoteWidget::onServerInfoResponse);
  this->onServerInfoReceivedCallback->SetClientData(this);

}

//-----------------------------------------------------------------------------
// qMRMLSegmentEditorWidget methods

//-----------------------------------------------------------------------------
qMRMLPlusLauncherRemoteWidget::qMRMLPlusLauncherRemoteWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLPlusLauncherRemoteWidgetPrivate(*this))
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->init();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
qMRMLPlusLauncherRemoteWidget::~qMRMLPlusLauncherRemoteWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  if (newScene == this->mrmlScene())
  {
    return;
  }

  Superclass::setMRMLScene(newScene);

  d->configFileSelectorComboBox->setMRMLScene(this->mrmlScene());

  // Make connections that depend on the Slicer application
  //QObject::connect(qSlicerApplication::application()->layoutManager(), SIGNAL(layoutChanged(int)), this, SLOT(onLayoutChanged(int)));

  // Update UI
  this->updateWidgetFromMRML();

  // observe close event so can re-add a parameters node if necessary
  //this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndCloseEvent()));
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  if (!this->mrmlScene() || this->mrmlScene()->IsClosing() || !d->ParameterSetNode)
  {
    bool checkBoxSignals = d->launcherConnectCheckBox->blockSignals(true);
    d->launcherConnectCheckBox->setDisabled(true);
    d->launcherConnectCheckBox->setChecked(false);
    d->launcherConnectCheckBox->blockSignals(checkBoxSignals);
    return;
  }

  int state = vtkMRMLIGTLConnectorNode::StateOff;
  vtkMRMLIGTLConnectorNode* launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (launcherConnectorNode)
  {
    state = launcherConnectorNode->GetState();
  }

  ///
  bool connectionEnabled = state != vtkMRMLIGTLConnectorNode::StateOff;

  bool checkBoxSignals = d->launcherConnectCheckBox->blockSignals(true);
  d->launcherConnectCheckBox->setEnabled(true);
  d->launcherConnectCheckBox->setChecked(connectionEnabled);
  d->launcherConnectCheckBox->blockSignals(checkBoxSignals);

  d->hostnameLineEdit->setEnabled(!connectionEnabled);
  d->portLineEdit->setEnabled(!connectionEnabled);

  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();
  if (configFileNode)
  {
    d->configFileTextEdit->setText(configFileNode->GetText());
  }
  else
  {
    d->configFileTextEdit->setText("");
  }

  bool connected = state == vtkMRMLIGTLConnectorNode::StateConnected;

  if (connectionEnabled)
  {
    if (connected)
    {
      if (d->launcherErrorLevel == LOG_LEVEL_ERROR)
      {
        d->launcherStatusLabel->setPixmap(d->IconRedDot);
      }
      else if (d->launcherErrorLevel == LOG_LEVEL_WARNING)
      {
        d->launcherStatusLabel->setPixmap(d->IconOrangeDot);
      }
      else
      {
        d->launcherStatusLabel->setPixmap(d->IconGreenDot);
      }
    }
    else
    {
      d->launcherStatusLabel->setPixmap(d->IconGreyDot);
    }
  }
  else
  {
    d->launcherStatusLabel->setPixmap(d->IconGreyDot);
  }

  bool configFileSelected = configFileNode != NULL;

  std::string hostname = d->ParameterSetNode->GetHostname();
  d->hostnameLineEdit->setText(QString::fromStdString(hostname));

  int port = d->ParameterSetNode->GetServerLauncherPort();
  d->portLineEdit->setText(QString::number(port));

  ///
  int serverState = d->ParameterSetNode->GetServerState();
  switch (serverState)
  {
  case vtkMRMLPlusRemoteLauncherNode::ServerOff:
  default:
    d->startStopServerButton->setEnabled(connected && configFileSelected);
    d->startStopServerButton->setText("Launch server");
    d->configFileSelectorComboBox->setEnabled(true);
    d->logLevelComboBox->setEnabled(true);
    d->serverStatusLabel->setPixmap(d->IconGreyDot);
    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerRunning:
    d->startStopServerButton->setEnabled(true);
    d->startStopServerButton->setText("Stop server");
    d->configFileSelectorComboBox->setDisabled(true);
    d->logLevelComboBox->setDisabled(true);
    if (d->serverErrorLevel == LOG_LEVEL_WARNING)
      d->serverStatusLabel->setPixmap(d->IconOrangeDot);
    else if (d->serverErrorLevel == LOG_LEVEL_ERROR)
      d->serverStatusLabel->setPixmap(d->IconRedDot);
    else
      d->serverStatusLabel->setPixmap(d->IconGreenDot);

    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerStarting:
    d->startStopServerButton->setDisabled(true);
    d->startStopServerButton->setText("Launching...");
    d->configFileSelectorComboBox->setDisabled(true);
    d->logLevelComboBox->setDisabled(true);
    d->serverStatusLabel->setPixmap(d->IconGreyDot);
    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerStopping:
    d->startStopServerButton->setDisabled(true);
    d->startStopServerButton->setText("Stopping...");
    d->configFileSelectorComboBox->setDisabled(true);
    d->logLevelComboBox->setDisabled(true);
    d->serverStatusLabel->setPixmap(d->IconGreyDot);
    break;
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onLauncherConnectorNodeModified()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onConnectCheckBoxChanged(bool connect)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  std::string hostnameString = d->hostnameLineEdit->text().toStdString();
  d->ParameterSetNode->SetHostname(hostnameString);
  const char* hostname = hostnameString.c_str();

  int port = d->portLineEdit->text().toInt();
  d->ParameterSetNode->SetServerLauncherPort(port);

  vtkMRMLIGTLConnectorNode* launcherConnectorNode = NULL;
  launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();

  // If there is a connector node selected, but the hostname or port doesn't match, stop observing the connector
  if (launcherConnectorNode)
  {
    if (strcmp(launcherConnectorNode->GetServerHostname(), hostname) != 0 || launcherConnectorNode->GetServerPort() != port)
    {
      launcherConnectorNode = NULL;
    }
  }

  if (!launcherConnectorNode)
  {
    std::vector<vtkMRMLNode*> connectorNodes = std::vector<vtkMRMLNode*>();
    //TODO find static method to get node type name
    this->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", connectorNodes);

    for (std::vector<vtkMRMLNode*>::iterator connectorNodeIt = connectorNodes.begin(); connectorNodeIt != connectorNodes.end(); ++connectorNodeIt)
    {
      vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*connectorNodeIt);
      if (!connectorNode)
      {
        continue;
      }

      if (strcmp(connectorNode->GetServerHostname(), hostname) == 0 && connectorNode->GetServerPort() == port)
      {
        launcherConnectorNode = connectorNode;
        break;
      }
    }
  }

  //TODO find static method to get node type name
  if (connect)
  {
    if (!launcherConnectorNode)
    {
      vtkMRMLNode* node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLIGTLConnectorNode", "PlusServerLauncherConnector");
      launcherConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    }

    if (launcherConnectorNode && launcherConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateOff)
    {
      launcherConnectorNode->Stop();
    }

    launcherConnectorNode->SetServerHostname(hostname);
    launcherConnectorNode->SetServerPort(port);
    launcherConnectorNode->Start();

  }
  else
  {
    if (launcherConnectorNode)
    {
      launcherConnectorNode->Stop();
    }
  }

  this->setAndObserveLauncherConnectorNode(launcherConnectorNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setAndObserveLauncherConnectorNode(vtkMRMLIGTLConnectorNode* connectorNode)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  d->ParameterSetNode->SetAndObserveLauncherConnectorNode(connectorNode);

  if (!connectorNode)
  {
    qvtkDisconnect(d->LauncherConnectorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkDisconnect(d->LauncherConnectorNode, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkDisconnect(d->LauncherConnectorNode, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(updateWidgetFromMRML()));
    d->LauncherConnectorNode->RemoveObserver(d->commandReceivedCallback);
    d->LauncherConnectorNode = NULL;
    return;
  }

  if (d->LauncherConnectorNode != connectorNode)
  {
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkMRMLIGTLConnectorNode::ConnectedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkReconnect(d->LauncherConnectorNode, connectorNode, vtkMRMLIGTLConnectorNode::DisconnectedEvent, this, SLOT(updateWidgetFromMRML()));

    if (d->LauncherConnectorNode)
    {
      d->LauncherConnectorNode->RemoveObserver(d->commandReceivedCallback);
    }

    d->LauncherConnectorNode = connectorNode;

    if (d->LauncherConnectorNode)
    {
      d->LauncherConnectorNode->AddObserver(vtkMRMLIGTLConnectorNode::CommandReceivedEvent, d->commandReceivedCallback);
    }

  }
}

//------------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onLoadConfigFile()
{
  QString filename = QFileDialog::getOpenFileName(NULL, "Select config file", "", "Config Files (*.xml);;AllFiles (*)");
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
  {
    return;
  }

  QFileInfo fileInfo(file);
  std::string name = fileInfo.fileName().toStdString();
  QString contents = file.readAll();

  vtkSmartPointer<vtkMRMLTextNode> configFileNode = vtkSmartPointer<vtkMRMLTextNode>::New();
  configFileNode->SetAttribute(CONFIG_FILE_NODE_ATTRIBUTE, "true");
  configFileNode->SetText(contents.toStdString().c_str());
  configFileNode->SetName(name.c_str());
  this->mrmlScene()->AddNode(configFileNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onConfigFileChanged(vtkMRMLNode* currentNode)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLTextNode* configFileNode = vtkMRMLTextNode::SafeDownCast(d->configFileSelectorComboBox->currentNode());
  d->ParameterSetNode->SetAndObserveCurrentConfigNode(configFileNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onStartStopButton()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusRemoteLauncherNode::ServerOff)
  {
    this->onLaunchServer();
  }
  else if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusRemoteLauncherNode::ServerRunning)
  {
    this->onStopServer();
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onClearLogButton()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->serverLogTextEdit->setPlainText("");
  d->serverErrorLevel = LOG_LEVEL_INFO;
  d->launcherErrorLevel = LOG_LEVEL_INFO;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onLaunchServer()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();

  if (!connectorNode || !configFileNode)
  {
    return;
  }

  int logLevel = d->logLevelComboBox->currentData().toInt();
  const char* fileName = configFileNode->GetName();
  const char* configFile = configFileNode->GetText();

  // TODO: Update with proper command syntax when finalized
  std::stringstream commandText;
  commandText << "<Command>" << std::endl;
  commandText << "  <LogLevel Value= \""<< logLevel << "\"/>" << std::endl;
  commandText << "  <FileName Value= \"" << fileName << "\"/>" << std::endl;
  commandText << configFile << std::endl;
  commandText << "</Command>" << std::endl;

  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> startServerCommand = vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New();
  startServerCommand->BlockingOff();
  startServerCommand->SetCommandName("StartServer");
  startServerCommand->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);
  startServerCommand->SetCommandText(commandText.str().c_str());
  startServerCommand->SetCommandTimeoutSec(1.0);
  startServerCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, d->startServerCallback);
  connectorNode->SendCommand(startServerCommand);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onStopServer()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // TODO: Update with proper command syntax when finalized
  std::stringstream commandText;
  commandText << "<Command><StopServer/></Command>" << std::endl;

  //TODO: device name based on parameter node name?
  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> stopServerCommand = vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New();
  stopServerCommand->BlockingOff();
  stopServerCommand->SetCommandName("StopServer");
  stopServerCommand->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);
  stopServerCommand->SetCommandText(commandText.str().c_str());
  stopServerCommand->SetCommandTimeoutSec(1.0);
  stopServerCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, d->stopServerCallback);
  connectorNode->SendCommand(stopServerCommand);

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onStartServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  vtkSlicerOpenIGTLinkCommand* startServerCommand = vtkSlicerOpenIGTLinkCommand::SafeDownCast(caller);
  if (!startServerCommand)
  {
    return;
  }

  qMRMLPlusLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  startServerCommand->RemoveObserver(d->startServerCallback);
  if (startServerCommand->IsSucceeded())
  {
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerStarting);
    d->q_ptr->getServerInfo();
  }
  else
  {
    // Expired or returned with failure.
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerOff);
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onStopServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  vtkSlicerOpenIGTLinkCommand* stopServerCommand = vtkSlicerOpenIGTLinkCommand::SafeDownCast(caller);
  if (!stopServerCommand)
  {
    return;
  }

  qMRMLPlusLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  stopServerCommand->RemoveObserver(d->stopServerCallback);

  if (stopServerCommand->IsSucceeded())
  {
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerStopping);
  }
  else
  {
    // Expired or retruned with failure
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerRunning);
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{
  qMRMLPlusLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }

  vtkSlicerOpenIGTLinkCommand* command = static_cast<vtkSlicerOpenIGTLinkCommand*>(calldata);
  if (!command)
  {
    return;
  }

  const char* name = command->GetCommandName();
  vtkSmartPointer<vtkXMLDataElement> rootElement = vtkXMLUtilities::ReadElementFromString(command->GetCommandText());

  if (strcmp(name, "ServerStarted") == 0)
  {
    d->serverErrorLevel = LOG_LEVEL_INFO;
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerRunning);
  }
  else if (strcmp(name, "ServerStopped") == 0)
  {
    d->serverErrorLevel = LOG_LEVEL_INFO;
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerOff);
  }
  else if (strcmp(name, "LogMessage") == 0)
  {
    d->q_ptr->onLogMessageCommand(rootElement);
  }

  command->SetResponseText("<Command><Result Success=\"TRUE\"/></Command>");
  d->ParameterSetNode->GetLauncherConnectorNode()->SendCommandResponse(command);

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onLogMessageCommand(vtkXMLDataElement* messageCommand)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  for (int i = 0; i < messageCommand->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* nestedElement = messageCommand->GetNestedElement(i);
    if (strcmp(nestedElement->GetName(), "LogMessage") != 0)
    {
      continue;
    }

    const char* messageContents = nestedElement->GetAttribute("Text");
    if (!messageContents)
    {
      return;
    }

    int logLevel = 0;
    if (!nestedElement->GetScalarAttribute("LogLevel", logLevel))
    {
      logLevel = LOG_LEVEL_INFO;
    }

    std::stringstream message;
    if (logLevel == LOG_LEVEL_ERROR)
    {
      message << "<font color = \"" << COLOR_ERROR << "\">";
    }
    else if (logLevel == LOG_LEVEL_WARNING)
    {
      message << "<font color = \"" << COLOR_WARNING << "\">";
    }
    else
    {
      message << "<font color = \"" << COLOR_NORMAL << "\">";
    }
    message << messageContents << "<br / >";
    message << "</font>";

    const char* messageOrigin = nestedElement->GetAttribute("Origin");
    bool logLevelChanged = false;
    if (messageOrigin && strcmp(messageOrigin, "SERVER") == 0)
    {
      if (logLevel < d->serverErrorLevel)
      {
        d->serverErrorLevel = logLevel;
        logLevelChanged = true;
      }
    }
    else
    {
      if (logLevel < d->launcherErrorLevel)
      {
        d->launcherErrorLevel = logLevel;
        logLevelChanged = true;
      }
    }

    // TODO: tracking position behavior:: see QPlusStatusIcon::ParseMessage
    d->serverLogTextEdit->moveCursor(QTextCursor::End);
    d->serverLogTextEdit->insertHtml(QString::fromStdString(message.str()));
    d->serverLogTextEdit->moveCursor(QTextCursor::End);

    if (logLevelChanged)
    {
      this->updateWidgetFromMRML();
    }

  }


}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::getServerInfo()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (!connectorNode)
  {
    return;
  }

  // TODO: Update with proper command syntax when finalized
  std::stringstream commandText;
  commandText << "<Command>" << std::endl;
  commandText << "  <PlusOpenIGTLinkServer/>" << std::endl;
  commandText << "</Command>" << std::endl;

  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> getServerInfoCommand = vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New();
  getServerInfoCommand->BlockingOff();
  getServerInfoCommand->SetCommandName("GetServerInfo");
  getServerInfoCommand->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);
  getServerInfoCommand->SetCommandText(commandText.str().c_str());
  getServerInfoCommand->SetCommandTimeoutSec(1.0);
  getServerInfoCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, d->onServerInfoReceivedCallback);
  connectorNode->SendCommand(getServerInfoCommand);

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onServerInfoResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
{

  vtkSlicerOpenIGTLinkCommand* getServerInfoCommand = vtkSlicerOpenIGTLinkCommand::SafeDownCast(caller);
  if (!getServerInfoCommand)
  {
    return;
  }

  qMRMLPlusLauncherRemoteWidgetPrivate* d = static_cast<qMRMLPlusLauncherRemoteWidgetPrivate*>(clientdata);
  if (!d)
  {
    return;
  }
  getServerInfoCommand->RemoveObserver(d->stopServerCallback);

  if (getServerInfoCommand->IsSucceeded())
  {
    vtkXMLDataElement* getServerInfoElement = getServerInfoCommand->GetResponseXML();
    for (int i = 0; i < getServerInfoElement->GetNumberOfNestedElements(); ++i)
    {
      vtkXMLDataElement* nestedElement = getServerInfoElement->GetNestedElement(i);
      if (strcmp(nestedElement->GetName(), "PlusOpenIGTLinkServer") == 0)
      {
        const char* id = nestedElement->GetAttribute("OutputChannelId");
        int port = 0;

        if (id && nestedElement->GetScalarAttribute("ListeningPort", port))
        {
          std::string hostnameString = d->ParameterSetNode->GetHostname();
          d->q_ptr->createConnectorNode(id, hostnameString.c_str(), d->ParameterSetNode->GetServerLauncherPort());
        }
      }
    }

  }
}

//-----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* qMRMLPlusLauncherRemoteWidget::createConnectorNode(const char* name, const char* hostname, int port)
{

  vtkMRMLIGTLConnectorNode* launcherConnectorNode = NULL;

  std::vector<vtkMRMLNode*> connectorNodes = std::vector<vtkMRMLNode*>();
  this->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", connectorNodes);

  for (std::vector<vtkMRMLNode*>::iterator connectorNodeIt = connectorNodes.begin(); connectorNodeIt != connectorNodes.end(); ++connectorNodeIt)
  {
    vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*connectorNodeIt);
    if (!connectorNode)
    {
      continue;
    }

    if (strcmp(connectorNode->GetServerHostname(), hostname) == 0 && connectorNode->GetServerPort() == port)
    {
      launcherConnectorNode = connectorNode;
      break;
    }
  }

  if (!launcherConnectorNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLIGTLConnectorNode", "PlusServerLauncherConnector");
    launcherConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  }

  if (launcherConnectorNode && launcherConnectorNode->GetState() != vtkMRMLIGTLConnectorNode::StateOff)
  {
    launcherConnectorNode->Stop();
  }

  launcherConnectorNode->SetName(name);
  launcherConnectorNode->SetServerHostname(hostname);
  launcherConnectorNode->SetServerPort(port);
  launcherConnectorNode->Start();

  return launcherConnectorNode;
}

//-----------------------------------------------------------------------------
bool qMRMLPlusLauncherRemoteWidget::advancedOptionsVisible() const
{
  Q_D(const qMRMLPlusLauncherRemoteWidget);
  return d->advancedGroupBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setAdvancedOptionsVisible(bool visible)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->advancedGroupBox->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qMRMLPlusLauncherRemoteWidget::logVisible() const
{
  Q_D(const qMRMLPlusLauncherRemoteWidget);
  return d->logGroupBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setLogVisible(bool visible)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->logGroupBox->setVisible(visible);
}

//------------------------------------------------------------------------------
vtkMRMLPlusRemoteLauncherNode* qMRMLPlusLauncherRemoteWidget::plusRemoteLauncherNode()const
{
  Q_D(const qMRMLPlusLauncherRemoteWidget);
  return d->ParameterSetNode;
}


//------------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setPlusRemoteLauncherNode(vtkMRMLPlusRemoteLauncherNode* newPlusRemoteLauncherNode)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  if (d->ParameterSetNode == newPlusRemoteLauncherNode)
  {
    return;
  }

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, newPlusRemoteLauncherNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set parameter set node
  d->ParameterSetNode = newPlusRemoteLauncherNode;

  if (!d->ParameterSetNode)
  {
    return;
  }

  // Update UI
  this->updateWidgetFromMRML();
}