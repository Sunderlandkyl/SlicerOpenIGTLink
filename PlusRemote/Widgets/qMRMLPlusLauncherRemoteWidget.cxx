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

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMRMLAbstractLogic.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>
#include <vtkCollection.h>

// MRML includes
#include <vtkMRMLScene.h>

// OpenIGTLinkIF includes
#include <vtkMRMLTextNode.h>
#include <vtkMRMLTextStorageNode.h>
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkSlicerOpenIGTLinkCommand.h>

//-----------------------------------------------------------------------------
const char* CONFIG_FILE_NODE_ATTRIBUTE = "ConfigFile";

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

  static void onStartServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onStopServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  static void onServerInfoResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata);
  void onLogMessageCommand(vtkXMLDataElement* messageCommandElement);

  virtual void getServerInfo();

  vtkSmartPointer<vtkMRMLIGTLConnectorNode> createConnectorNode(const char* id, const char* hostname, int port);

public:

  //TODO: look up QT style for member variables, etc.
  vtkSmartPointer<vtkCallbackCommand> StartServerCallback;
  vtkSmartPointer<vtkCallbackCommand> StopServerCallback;
  vtkSmartPointer<vtkCallbackCommand> CommandReceivedCallback;
  vtkSmartPointer<vtkCallbackCommand> ServerInfoReceivedCallback;

  vtkWeakPointer<vtkMRMLPlusRemoteLauncherNode> ParameterSetNode;
  vtkWeakPointer<vtkMRMLIGTLConnectorNode> LauncherConnectorNode;

  vtkSmartPointer<vtkMRMLTextNode> LogTextNode;

  QPixmap IconDisconnected;
  QPixmap IconNotConnected;
  QPixmap IconConnected;
  QPixmap IconWaiting;
  QPixmap IconRunning;
  QPixmap IconRunningWarning;
  QPixmap IconRunningError;

  int CurrentErrorLevel;
};

//-----------------------------------------------------------------------------
qMRMLPlusLauncherRemoteWidgetPrivate::qMRMLPlusLauncherRemoteWidgetPrivate(qMRMLPlusLauncherRemoteWidget& object)
  : q_ptr(&object)
  , LauncherConnectorNode(NULL)
{
  this->IconDisconnected = QPixmap(":/Icons/PlusLauncherRemoteDisconnected.png");
  this->IconNotConnected = QPixmap(":/Icons/PlusLauncherRemoteNotConnected.png");
  this->IconConnected = QPixmap(":/Icons/PlusLauncherRemoteConnect.png");
  this->IconWaiting = QPixmap(":/Icons/PlusLauncherRemoteWaitForConnection.png");
  this->IconRunning = QPixmap(":/Icons/PlusLauncherRemoteRunning.png");
  this->IconRunningWarning = QPixmap(":/Icons/PlusLauncherRemoteRunningWarning.png");
  this->IconRunningError = QPixmap(":/Icons/PlusLauncherRemoteRunningError.png");
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

  this->ConfigFileSelectorComboBox->addAttribute("vtkMRMLTextNode", CONFIG_FILE_NODE_ATTRIBUTE);

  this->CurrentErrorLevel = vtkMRMLPlusRemoteLauncherNode::LogLevelInfo;

  this->StartServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->StartServerCallback->SetCallback(qMRMLPlusLauncherRemoteWidgetPrivate::onStartServerResponse);
  this->StartServerCallback->SetClientData(this);

  this->StopServerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->StopServerCallback->SetCallback(qMRMLPlusLauncherRemoteWidgetPrivate::onStopServerResponse);
  this->StopServerCallback->SetClientData(this);

  this->CommandReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->CommandReceivedCallback->SetCallback(qMRMLPlusLauncherRemoteWidgetPrivate::onCommandReceived);
  this->CommandReceivedCallback->SetClientData(this);

  this->ServerInfoReceivedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->ServerInfoReceivedCallback->SetCallback(qMRMLPlusLauncherRemoteWidgetPrivate::onServerInfoResponse);
  this->ServerInfoReceivedCallback->SetClientData(this);

  this->LogLevelComboBox->addItem("Error", vtkMRMLPlusRemoteLauncherNode::LogLevelError);
  this->LogLevelComboBox->addItem("Warning", vtkMRMLPlusRemoteLauncherNode::LogLevelWarning);
  this->LogLevelComboBox->addItem("Info", vtkMRMLPlusRemoteLauncherNode::LogLevelInfo);
  this->LogLevelComboBox->addItem("Debug", vtkMRMLPlusRemoteLauncherNode::LogLevelDebug);
  //this->LogLevelComboBox->addItem("Trace", vtkMRMLPlusRemoteLauncherNode::LogLevelTrace); // Callback in vtkPlusLogger does not trigger on trace

  QObject::connect(this->LauncherConnectCheckBox, SIGNAL(toggled(bool)), q, SLOT(onConnectCheckBoxChanged(bool)));
  QObject::connect(this->LoadConfigFileButton, SIGNAL(clicked()), q, SLOT(onLoadConfigFile()));
  QObject::connect(this->ConfigFileSelectorComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(onConfigFileChanged(vtkMRMLNode*)));
  QObject::connect(this->LogLevelComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onLogLevelChanged(int)));
  QObject::connect(this->StartStopServerButton, SIGNAL(clicked()), q, SLOT(onStartStopButton()));
  QObject::connect(this->ClearLogButton, SIGNAL(clicked()), q, SLOT(onClearLogButton()));
  QObject::connect(this->HostnameLineEdit, SIGNAL(textEdited(const QString &)), q, SLOT(onHostChanged(const QString &)));
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkMRMLIGTLConnectorNode> qMRMLPlusLauncherRemoteWidgetPrivate::createConnectorNode(const char* name, const char* hostname, int port)
{
  Q_Q(qMRMLPlusLauncherRemoteWidget);

  vtkMRMLIGTLConnectorNode* launcherConnectorNode = NULL;

  std::vector<vtkMRMLNode*> connectorNodes = std::vector<vtkMRMLNode*>();
  q->mrmlScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", connectorNodes);

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
    vtkMRMLNode* node = q->mrmlScene()->AddNewNodeByClass("vtkMRMLIGTLConnectorNode", "PlusServerLauncherConnector");
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
void qMRMLPlusLauncherRemoteWidgetPrivate::onStartServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
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
  startServerCommand->RemoveObserver(d->StartServerCallback);

  if (startServerCommand->IsSucceeded())
  {
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerStarting);
    vtkSmartPointer<vtkXMLDataElement> startServerResponseElement = startServerCommand->GetResponseXML();
    // TODO: check for success
  }
  else
  {
    // Expired or returned with failure.
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerOff);
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidgetPrivate::onStopServerResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
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
  stopServerCommand->RemoveObserver(d->StopServerCallback);

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
void qMRMLPlusLauncherRemoteWidgetPrivate::onCommandReceived(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
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

  std::string name = command->GetCommandName();
  vtkSmartPointer<vtkXMLDataElement> rootElement = vtkXMLUtilities::ReadElementFromString(command->GetCommandText().c_str());

  if (name == "ServerStarted")
  {
    d->CurrentErrorLevel = vtkMRMLPlusRemoteLauncherNode::LogLevelInfo;
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerRunning);
    vtkSmartPointer<vtkXMLDataElement> plusConfigurationResponseElement = rootElement->FindNestedElementWithName("ServerStarted");
    if (plusConfigurationResponseElement)
    {
      const char* ports = plusConfigurationResponseElement->GetAttribute("Servers");
      if (ports)
      {
        //std::vector<std::string> tokens = std::vector<std::string>();
        std::istringstream ss(ports);
        std::string nameAndPortTokentoken;


        while (std::getline(ss, nameAndPortTokentoken, ';')) {

          std::string token;
          std::istringstream innerSS(nameAndPortTokentoken);
          std::vector<std::string> tokens = std::vector<std::string>();
          while (std::getline(innerSS, token, ':'))
          {
            tokens.push_back(token);
          }

          if (tokens.size() != 2)
          {
            continue;
          }

          std::string nameString = tokens[0];
          std::string portString = tokens[1];

          bool success;
          int port = QString::fromStdString(portString).toInt(&success);
          if (success)
          {
            std::string connectorName = d->ParameterSetNode->GetServerLauncherHostname() + std::string(" || ") + nameAndPortTokentoken;
            d->createConnectorNode(connectorName.c_str(), d->ParameterSetNode->GetServerLauncherHostname(), port);
          }
        }
      }
    }
  }
  else if (name == "ServerStopped")
  {
    d->CurrentErrorLevel = vtkMRMLPlusRemoteLauncherNode::LogLevelInfo;
    d->ParameterSetNode->SetServerState(vtkMRMLPlusRemoteLauncherNode::ServerOff);
  }
  else if (name == "LogMessage")
  {
    d->onLogMessageCommand(rootElement);
  }

  command->SetResponseText("<Command><Result Success=\"TRUE\"/></Command>");

  vtkMRMLIGTLConnectorNode* connectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (connectorNode)
  {
    connectorNode->SendCommandResponse(command);
  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidgetPrivate::onLogMessageCommand(vtkXMLDataElement* messageCommand)
{
  Q_Q(qMRMLPlusLauncherRemoteWidget);

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
      logLevel = vtkMRMLPlusRemoteLauncherNode::LogLevelInfo;
    }

    std::stringstream message;
    if (logLevel == vtkMRMLPlusRemoteLauncherNode::LogLevelError)
    {
      message << "<font color = \"" << COLOR_ERROR << "\">";
    }
    else if (logLevel == vtkMRMLPlusRemoteLauncherNode::LogLevelWarning)
    {
      message << "<font color = \"" << COLOR_WARNING << "\">";
    }
    else
    {
      message << "<font color = \"" << COLOR_NORMAL << "\">";
    }
    message << messageContents << "<br / >";
    message << "</font>";

    bool logLevelChanged = false;
    if (logLevel < this->CurrentErrorLevel)
    {
      this->CurrentErrorLevel = logLevel;
      logLevelChanged = true;
    }

    // TODO: tracking position behavior:: see QPlusStatusIcon::ParseMessage
    this->ServerLogTextEdit->moveCursor(QTextCursor::End);
    this->ServerLogTextEdit->insertHtml(QString::fromStdString(message.str()));
    this->ServerLogTextEdit->moveCursor(QTextCursor::End);

    if (logLevelChanged)
    {
      q->updateWidgetFromMRML();
    }

  }

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidgetPrivate::getServerInfo()
{
  vtkMRMLIGTLConnectorNode* connectorNode = this->ParameterSetNode->GetLauncherConnectorNode();
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
  getServerInfoCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, this->ServerInfoReceivedCallback);
  connectorNode->SendCommand(getServerInfoCommand);

}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidgetPrivate::onServerInfoResponse(vtkObject* caller, unsigned long eid, void* clientdata, void *calldata)
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
  getServerInfoCommand->RemoveObserver(d->StopServerCallback);

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
          const char* hostname = d->ParameterSetNode->GetServerLauncherHostname();
          d->createConnectorNode(id, hostname, d->ParameterSetNode->GetServerLauncherPort());
        }
      }
    }

  }
}

//-----------------------------------------------------------------------------
// qMRMLPlusLauncherRemoteWidget methods

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

  d->ConfigFileSelectorComboBox->setMRMLScene(this->mrmlScene());

  // Make connections that depend on the Slicer application
  //QObject::connect(qSlicerApplication::application()->layoutManager(), SIGNAL(layoutChanged(int)), this, SLOT(onLayoutChanged(int)));

  // Update UI
  this->updateWidgetFromMRML();

  // observe close event so can re-add a parameters node if necessary
  this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndCloseEvent()));
}

////------------------------------------------------------------------------------
//void qMRMLPlusLauncherRemoteWidget::initializeParameterSetNode()
//{
//  Q_D(qMRMLPlusLauncherRemoteWidget);
//  if (!d->ParameterSetNode)
//  {
//    return;
//  }
//}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onMRMLSceneEndCloseEvent()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  
  this->setParameterSetNode(NULL);
  //this->initializeParameterSetNode();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  std::string tooltipSuffix = " Click to view log";
  if (!this->mrmlScene() || this->mrmlScene()->IsClosing() || !d->ParameterSetNode)
  {
    bool checkBoxSignals = d->LauncherConnectCheckBox->blockSignals(true);
    d->LauncherConnectCheckBox->setDisabled(true);
    d->LauncherConnectCheckBox->setChecked(false);
    d->LauncherConnectCheckBox->blockSignals(checkBoxSignals);
    d->LauncherStatusButton->setIcon(d->IconDisconnected);
    d->LauncherStatusButton->setToolTip(QString::fromStdString("Launcher disconnected. " + tooltipSuffix));
    d->ConfigFileTextEdit->setText("");
    return;
  }

  int disabledModify = d->ParameterSetNode->StartModify();

  int state = vtkMRMLIGTLConnectorNode::StateOff;
  vtkMRMLIGTLConnectorNode* launcherConnectorNode = d->ParameterSetNode->GetLauncherConnectorNode();
  if (launcherConnectorNode)
  {
    state = launcherConnectorNode->GetState();
  }

  ///
  bool connectionEnabled = state != vtkMRMLIGTLConnectorNode::StateOff;

  bool checkBoxSignals = d->LauncherConnectCheckBox->blockSignals(true);
  d->LauncherConnectCheckBox->setEnabled(true);
  d->LauncherConnectCheckBox->setChecked(connectionEnabled);
  d->LauncherConnectCheckBox->blockSignals(checkBoxSignals);

  d->HostnameLineEdit->setEnabled(!connectionEnabled);

  vtkMRMLTextNode* configFileNode = d->ParameterSetNode->GetCurrentConfigNode();
  if (configFileNode)
  {
    d->ConfigFileTextEdit->setText(configFileNode->GetText());
  }
  else
  {
    d->ConfigFileTextEdit->setText("");
  }

  bool connected = state == vtkMRMLIGTLConnectorNode::StateConnected;
  if (connectionEnabled)
  {

    if (connected)
    {
      int serverState = d->ParameterSetNode->GetServerState();
      switch (serverState)
      {
        case vtkMRMLPlusRemoteLauncherNode::ServerRunning:
          if (d->CurrentErrorLevel == vtkMRMLPlusRemoteLauncherNode::LogLevelInfo)
          {
            d->LauncherStatusButton->setIcon(d->IconRunning);
            d->LauncherStatusButton->setToolTip(QString::fromStdString("Server runnning." + tooltipSuffix));
          }
          else if (d->CurrentErrorLevel == vtkMRMLPlusRemoteLauncherNode::LogLevelError)
          {
            d->LauncherStatusButton->setIcon(d->IconRunningError);
            d->LauncherStatusButton->setToolTip(QString::fromStdString("Server runnning. Error detected. " + tooltipSuffix));
          }
          else if (d->CurrentErrorLevel == vtkMRMLPlusRemoteLauncherNode::LogLevelWarning)
            d->LauncherStatusButton->setIcon(d->IconRunningWarning);
            d->LauncherStatusButton->setToolTip(QString::fromStdString("Server runnning. Warning detected. " + tooltipSuffix));
          break;
        case vtkMRMLPlusRemoteLauncherNode::ServerStarting:
          d->LauncherStatusButton->setIcon(d->IconWaiting);
          d->LauncherStatusButton->setToolTip(QString::fromStdString("Server starting. " + tooltipSuffix));
          break;
        case vtkMRMLPlusRemoteLauncherNode::ServerStopping:
          d->LauncherStatusButton->setIcon(d->IconWaiting);
          d->LauncherStatusButton->setToolTip(QString::fromStdString("Server stopping. " + tooltipSuffix));
          break;
        case vtkMRMLPlusRemoteLauncherNode::ServerOff:
        default:
          d->LauncherStatusButton->setIcon(d->IconConnected);
          d->LauncherStatusButton->setToolTip(QString::fromStdString("Launcher connected. " + tooltipSuffix));
          break;
      }
    }
    else
    {
      d->LauncherStatusButton->setIcon(d->IconNotConnected);
      d->LauncherStatusButton->setToolTip(QString::fromStdString("Launcher not connected. " + tooltipSuffix));
    }

  }
  else
  {
    d->LauncherStatusButton->setIcon(d->IconDisconnected);
    d->LauncherStatusButton->setToolTip(QString::fromStdString("Launcher disconnected. " + tooltipSuffix));
  }

  bool configFileSelected = configFileNode != NULL;

  const char* hostname = d->ParameterSetNode->GetServerLauncherHostname();
  int port = d->ParameterSetNode->GetServerLauncherPort();
  int cursorPosition = d->HostnameLineEdit->cursorPosition();
  std::stringstream hostnameAndPort;
  hostnameAndPort << hostname << ":" << port;
  d->HostnameLineEdit->setText(QString::fromStdString(hostnameAndPort.str()));
  d->HostnameLineEdit->setCursorPosition(cursorPosition);

  ///
  int serverState = d->ParameterSetNode->GetServerState();
  switch (serverState)
  {
  case vtkMRMLPlusRemoteLauncherNode::ServerRunning:
    d->StartStopServerButton->setEnabled(true);
    d->StartStopServerButton->setText("Stop server");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerStarting:
    d->StartStopServerButton->setDisabled(true);
    d->StartStopServerButton->setText("Launching...");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerStopping:
    d->StartStopServerButton->setDisabled(true);
    d->StartStopServerButton->setText("Stopping...");
    d->ConfigFileSelectorComboBox->setDisabled(true);
    d->LogLevelComboBox->setDisabled(true);
    break;
  case vtkMRMLPlusRemoteLauncherNode::ServerOff:
  default:
    d->StartStopServerButton->setEnabled(connected && configFileSelected);
    d->StartStopServerButton->setText("Launch server");
    d->ConfigFileSelectorComboBox->setEnabled(true);
    d->LogLevelComboBox->setEnabled(true);
    break;
  }

  d->ConfigFileSelectorComboBox->setCurrentNode(d->ParameterSetNode->GetCurrentConfigNode());
  d->LogLevelComboBox->setCurrentIndex(d->LogLevelComboBox->findData(d->ParameterSetNode->GetLogLevel()));

  d->ParameterSetNode->EndModify(disabledModify);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onConnectCheckBoxChanged(bool connect)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  const char* hostname = d->ParameterSetNode->GetServerLauncherHostname();
  int port = d->ParameterSetNode->GetServerLauncherPort();

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
    d->LauncherConnectorNode->RemoveObserver(d->CommandReceivedCallback);
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
      d->LauncherConnectorNode->RemoveObserver(d->CommandReceivedCallback);
    }

    d->LauncherConnectorNode = connectorNode;

    if (d->LauncherConnectorNode)
    {
      d->LauncherConnectorNode->AddObserver(vtkMRMLIGTLConnectorNode::CommandReceivedEvent, d->CommandReceivedCallback);
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
  vtkSmartPointer<vtkMRMLTextNode> configFileNode = vtkSmartPointer<vtkMRMLTextNode>::New();

  configFileNode->SaveWithSceneOn();
  configFileNode->SetAttribute(CONFIG_FILE_NODE_ATTRIBUTE, "true");
  this->mrmlScene()->AddNode(configFileNode);

  vtkSmartPointer<vtkMRMLTextStorageNode> configFileStorageNode = vtkSmartPointer<vtkMRMLTextStorageNode>::New();

  this->mrmlScene()->AddNode(configFileStorageNode);
  configFileNode->SetAndObserveStorageNodeID(configFileStorageNode->GetID());
  
  std::string stringFilename = filename.toStdString();
  configFileStorageNode->SetFileName(stringFilename.c_str());
  configFileStorageNode->ReadData(configFileNode, true);

  std::string name = fileInfo.fileName().toStdString();

  const char* configFileText = configFileNode->GetText();
  

  if (!configFileText)
  {
    this->mrmlScene()->RemoveNode(configFileNode);
    this->mrmlScene()->RemoveNode(configFileStorageNode);
    return;
  }
  vtkSmartPointer<vtkXMLDataElement> configFileXML = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(configFileText));
  if (!configFileXML)
  {
    this->mrmlScene()->RemoveNode(configFileNode);
    this->mrmlScene()->RemoveNode(configFileStorageNode);
    return;
  }

  vtkSmartPointer<vtkXMLDataElement> dataCollectionElement = configFileXML->FindNestedElementWithName("DataCollection");
  if (dataCollectionElement)
  {
    vtkSmartPointer<vtkXMLDataElement> deviceSetElement = dataCollectionElement->FindNestedElementWithName("DeviceSet");
    if (deviceSetElement)
    {
      const char* nameAttribute = deviceSetElement->GetAttribute("Name");
      if (nameAttribute)
      {
        name = nameAttribute;
      }
    }

  }

  configFileNode->SetName(name.c_str());
  std::string storageNodeName = name + "_StorageNode";
  configFileStorageNode->SetName(storageNodeName.c_str());
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onConfigFileChanged(vtkMRMLNode* currentNode)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  vtkMRMLTextNode* configFileNode = vtkMRMLTextNode::SafeDownCast(d->ConfigFileSelectorComboBox->currentNode());
  d->ParameterSetNode->SetAndObserveCurrentConfigNode(configFileNode);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onLogLevelChanged(int index)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (!d->ParameterSetNode)
  {
    return;
  }

  int logLevel = d->LogLevelComboBox->currentData().toInt();
  d->ParameterSetNode->SetLogLevel(logLevel);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onHostChanged(const QString &text)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  std::string hostnameAndPort = text.toStdString();
  std::vector<std::string> tokens = std::vector<std::string>();
  std::istringstream ss(hostnameAndPort);
  std::string item;
  while (std::getline(ss, item, ':'))
  {
    tokens.push_back(item);
  }

  std::string hostname = d->ParameterSetNode->GetServerLauncherHostname();
  if (tokens.size() >= 1)
  {
    hostname = tokens[0];
  }

  int port = d->ParameterSetNode->GetServerLauncherPort();
  if (tokens.size() > 1)
  {
    bool success = false;
    int parsedPort = QVariant(tokens[1].c_str()).toInt(&success);
    if (success)
    {
      port = parsedPort;
    }
  }

  d->ParameterSetNode->SetServerLauncherHostname(hostname.c_str());
  d->ParameterSetNode->SetServerLauncherPort(port);

  hostname = d->ParameterSetNode->GetServerLauncherHostname();
  port = d->ParameterSetNode->GetServerLauncherPort();

  // If the input string contained some invalid characters, update widget from parameter node to remove them
  std::stringstream reformattedSS;
  reformattedSS << hostname << ":" << port;
  std::string reformattedString = reformattedSS.str();
  if (strcmp(reformattedString.c_str(), hostnameAndPort.c_str()) != 0)
  {
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onStartStopButton()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);

  if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusRemoteLauncherNode::ServerOff)
  {
    this->launchServer();
  }
  else if (d->ParameterSetNode->GetServerState() == vtkMRMLPlusRemoteLauncherNode::ServerRunning)
  {
    this->stopServer();
  }
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::onClearLogButton()
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->ServerLogTextEdit->setPlainText("");
  d->CurrentErrorLevel = vtkMRMLPlusRemoteLauncherNode::LogLevelInfo;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::launchServer()
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

  int logLevel = d->ParameterSetNode->GetLogLevel();
  const char* fileName = configFileNode->GetName();
  const char* configFile = configFileNode->GetText();
  // TODO: Update with proper command syntax when finalized
  std::stringstream commandText;
  commandText << "<Command>" << std::endl;
  commandText << "  <LogLevel Value= \""<< logLevel << "\"/>" << std::endl;
  //commandText << "  <FileName Value= \"" << fileName << "\"/>" << std::endl;
  if (configFile)
  {
    commandText << configFile << std::endl;
  }
  commandText << "</Command>" << std::endl;

  vtkSmartPointer<vtkSlicerOpenIGTLinkCommand> startServerCommand = vtkSmartPointer<vtkSlicerOpenIGTLinkCommand>::New();
  startServerCommand->BlockingOff();
  startServerCommand->SetCommandName("StartServer");
  startServerCommand->SetDeviceID(PLUS_SERVER_LAUNCHER_REMOTE_DEVICE_ID);
  startServerCommand->SetCommandText(commandText.str().c_str());
  startServerCommand->SetCommandTimeoutSec(1.0);
  startServerCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, d->StartServerCallback);
  connectorNode->SendCommand(startServerCommand);
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::stopServer()
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
  stopServerCommand->AddObserver(vtkSlicerOpenIGTLinkCommand::CommandCompletedEvent, d->StopServerCallback);
  connectorNode->SendCommand(stopServerCommand);

}

//-----------------------------------------------------------------------------
bool qMRMLPlusLauncherRemoteWidget::logVisible() const
{
  Q_D(const qMRMLPlusLauncherRemoteWidget);
  return d->LogGroupBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setLogVisible(bool visible)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  d->LogGroupBox->setVisible(visible);
}

//------------------------------------------------------------------------------
vtkMRMLPlusRemoteLauncherNode* qMRMLPlusLauncherRemoteWidget::plusRemoteLauncherNode()const
{
  Q_D(const qMRMLPlusLauncherRemoteWidget);
  return d->ParameterSetNode;
}


//------------------------------------------------------------------------------
void qMRMLPlusLauncherRemoteWidget::setParameterSetNode(vtkMRMLPlusRemoteLauncherNode* parameterNode)
{
  Q_D(qMRMLPlusLauncherRemoteWidget);
  if (d->ParameterSetNode == parameterNode)
  {
    return;
  }

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, parameterNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set parameter set node
  d->ParameterSetNode = parameterNode;

  if (!d->ParameterSetNode)
  {
    return;
  }

  // Update UI
  this->updateWidgetFromMRML();
}