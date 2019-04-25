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

#include "vtkMRMLPlusServerNode.h"

// PlusRemote includes
#include "vtkMRMLPlusServerLauncherNode.h"


// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

// STD includes
#include <sstream>

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkMRMLTextNode.h>
#include <vtkMRMLNode.h>

//------------------------------------------------------------------------------
const char* vtkMRMLPlusServerNode::CONFIG_REFERENCE_ROLE = "configNodeRef";
const char* vtkMRMLPlusServerNode::LAUNCHER_REFERENCE_ROLE = "launcherNodeRef";
const char* vtkMRMLPlusServerNode::PLUS_SERVER_CONNECTOR_REFERENCE_ROLE = "plusServerConnectorNodeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::vtkMRMLPlusServerNode()
  : DesiredState(ServerStatus::Off)
  , State(ServerStatus::Off)
  , LogLevel(LOG_INFO)
{
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::~vtkMRMLPlusServerNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLIntMacro(desiredState, DesiredState);
  vtkMRMLWriteXMLIntMacro(logLevel, LogLevel);
  vtkMRMLWriteXMLStdStringMacro(configFileName, ConfigFileName);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLIntMacro(desiredState, DesiredState);
  vtkMRMLReadXMLIntMacro(logLevel, LogLevel);
  vtkMRMLReadXMLStdStringMacro(configFileName, ConfigFileName);
  vtkMRMLReadXMLEndMacro();
  this->UpdateConfigFileInfo();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyIntMacro(DesiredState);
  vtkMRMLCopyIntMacro(LogLevel);
  vtkMRMLCopyStdStringMacro(ConfigFileName);
  vtkMRMLCopyEndMacro();
  this->UpdateConfigFileInfo();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintIntMacro(DesiredState);
  vtkMRMLPrintIntMacro(LogLevel);
  vtkMRMLPrintStdStringMacro(ConfigFileName);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  if (eventID == vtkCommand::ModifiedEvent
    && caller == this->GetConfigNode())
  {
    // Update the config file info
    this->UpdateConfigFileInfo();
  }
}


//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode* vtkMRMLPlusServerNode::GetLauncherNode()
{
  return vtkMRMLPlusServerLauncherNode::SafeDownCast(this->GetNodeReference(LAUNCHER_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetAndObserveLauncherNode(vtkMRMLPlusServerLauncherNode* node)
{
  this->SetNodeReferenceID(LAUNCHER_REFERENCE_ROLE, (node ? node->GetID() : NULL));
  node->AddNodeReferenceID(vtkMRMLPlusServerLauncherNode::PLUS_SERVER_REFERENCE_ROLE, this->GetID());
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLTextNode* vtkMRMLPlusServerNode::GetConfigNode()
{
  return vtkMRMLTextNode::SafeDownCast(this->GetNodeReference(CONFIG_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::UpdateConfigFileInfo()
{
  int wasModifying = this->StartModify();

  vtkMRMLTextNode* configFileNode = this->GetConfigNode();
  if (configFileNode)
  {
    std::stringstream ss;
    ss << configFileNode->GetName();
    ss << ".slicer.xml";
    this->SetConfigFileName(ss.str());
  }

  std::string content;
  if (configFileNode && configFileNode->GetText())
  {
    content = configFileNode->GetText();
  }

  vtkMRMLPlusServerNode::PlusConfigFileInfo configInfo = vtkMRMLPlusServerNode::GetPlusConfigFileInfo(content);
  this->SetDeviceSetName(configInfo.Name);
  this->SetDeviceSetDescription(configInfo.Description);
  this->PlusOpenIGTLinkServers = configInfo.Servers;

  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetAndObserveConfigNode(vtkMRMLTextNode* configFileNode)
{
  this->SetNodeReferenceID(CONFIG_REFERENCE_ROLE, (configFileNode ? configFileNode->GetID() : NULL));

  int wasModifying = this->StartModify();
  this->UpdateConfigFileInfo();
  this->Modified();
  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::AddAndObservePlusOpenIGTLinkServerConnector(vtkMRMLIGTLConnectorNode* connectorNode)
{
  this->AddNodeReferenceID(PLUS_SERVER_CONNECTOR_REFERENCE_ROLE, (connectorNode ? connectorNode->GetID() : NULL));
}

//----------------------------------------------------------------------------
std::vector<vtkMRMLIGTLConnectorNode*> vtkMRMLPlusServerNode::GetPlusOpenIGTLinkConnectorNodes()
{
  std::vector<vtkMRMLIGTLConnectorNode*> connectorNodes;
  std::vector<vtkMRMLNode*> nodes;
  this->GetNodeReferences(vtkMRMLPlusServerNode::PLUS_SERVER_CONNECTOR_REFERENCE_ROLE, nodes);
  for (std::vector<vtkMRMLNode*>::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
  {
    vtkMRMLIGTLConnectorNode* connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(*nodeIt);
    if (!connectorNode)
    {
      continue;
    }
    connectorNodes.push_back(connectorNode);
  }

  return connectorNodes;
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::StartServer()
{
  this->SetDesiredState(ServerStatus::On);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::StopServer()
{
  this->SetDesiredState(ServerStatus::Off);
}


//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::PlusConfigFileInfo vtkMRMLPlusServerNode::GetPlusConfigFileInfo(std::string content)
{
  vtkMRMLPlusServerNode::PlusConfigFileInfo configFileInfo;

  vtkSmartPointer<vtkXMLDataElement> rootElement;
  if (!content.empty())
  {
    rootElement = vtkSmartPointer<vtkXMLDataElement>::Take(
      vtkXMLUtilities::ReadElementFromString(content.c_str()));
  }
  if (!rootElement)
  {
    return configFileInfo;
  }

  for (int i = 0; i < rootElement->GetNumberOfNestedElements(); ++i)
  {
    vtkSmartPointer<vtkXMLDataElement> nestedElement = rootElement->GetNestedElement(i);
    std::string nestedElementName = nestedElement->GetName();

    if (nestedElementName == "DataCollection")
    {
      vtkSmartPointer<vtkXMLDataElement> dataCollectionElement = nestedElement;
      for (int j = 0; j < dataCollectionElement->GetNumberOfNestedElements(); ++j)
      {
        vtkSmartPointer<vtkXMLDataElement> nestedDataCollectionElement = dataCollectionElement->GetNestedElement(j);
        std::string nestedDataCollectionElementName = nestedDataCollectionElement->GetName();
        if (nestedDataCollectionElementName == "DeviceSet")
        {
          if (nestedDataCollectionElement->GetAttribute("Name"))
          {
            configFileInfo.Name = nestedDataCollectionElement->GetAttribute("Name");
          }
          if (nestedDataCollectionElement->GetAttribute("Description"))
          {
            configFileInfo.Description = nestedDataCollectionElement->GetAttribute("Description");
          }
        }
      }
    }

    if (nestedElementName == "PlusOpenIGTLinkServer")
    {
      vtkSmartPointer<vtkXMLDataElement> plusOpenIGTLinkServerElement = nestedElement;

      vtkMRMLPlusServerNode::PlusOpenIGTLinkServerInfo serverInfo;
      serverInfo.ListeningPort = -1;
      if (plusOpenIGTLinkServerElement->GetAttribute("ListeningPort"))
      {
        serverInfo.ListeningPort = vtkVariant(plusOpenIGTLinkServerElement->GetAttribute("ListeningPort")).ToInt();
      }
      if (plusOpenIGTLinkServerElement->GetAttribute("OutputChannelId"))
      {
        serverInfo.OutputChannelId = plusOpenIGTLinkServerElement->GetAttribute("OutputChannelId");
      }
      configFileInfo.Servers.push_back(serverInfo);
    }
  }
  return configFileInfo;
}