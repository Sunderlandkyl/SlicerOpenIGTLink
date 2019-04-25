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

#ifndef __vtkMRMLPlusServerNode_h
#define __vtkMRMLPlusServerNode_h

// MRML includes
#include <vtkMRMLNode.h>

// vtkAddon includes  
#include <vtkAddonSetGet.h>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

class vtkMRMLScene;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLTextNode;
class vtkMRMLPlusServerLauncherNode;

/// \ingroup Segmentations
/// \brief Parameter set node for the plus remote launcher widget
///
class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkMRMLPlusServerNode : public vtkMRMLNode
{

public:

  enum ServerStatus
  {
    Off,
    On,
    Starting,
    Stopping,
  };

  static const char* CONFIG_REFERENCE_ROLE;
  static const char* LAUNCHER_REFERENCE_ROLE;
  static const char* PLUS_SERVER_CONNECTOR_REFERENCE_ROLE;

  static vtkMRMLPlusServerNode* New();
  vtkTypeMacro(vtkMRMLPlusServerNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Standard MRML node methods
  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(ostream& of, int indent) override;
  virtual void Copy(vtkMRMLNode* node) override;
  virtual const char* GetNodeTagName() override { return "PlusServer"; }

  void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData) override;

protected:
  vtkMRMLPlusServerNode();
  virtual ~vtkMRMLPlusServerNode();
  vtkMRMLPlusServerNode(const vtkMRMLPlusServerNode&);
  void operator=(const vtkMRMLPlusServerNode&);

public:

  enum
  {
    LOG_ERROR = 1,
    LOG_WARNING = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_TRACE = 5,
  };

  vtkGetMacro(DesiredState, int);
  vtkSetMacro(DesiredState, int);

  vtkGetMacro(State, int);
  vtkSetMacro(State, int);

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);

  vtkGetMacro(ConfigFileName, std::string);
  vtkSetMacro(ConfigFileName, std::string);

  vtkGetMacro(DeviceSetName, std::string);
  vtkSetMacro(DeviceSetName, std::string);

  vtkGetMacro(DeviceSetDescription, std::string);
  vtkSetMacro(DeviceSetDescription, std::string);

  void StartServer();
  void StopServer();

  /// Get config file node
  vtkMRMLTextNode* GetConfigNode();
  /// Set and observe config file node
  void SetAndObserveConfigNode(vtkMRMLTextNode* node);

  /// Get launcher node
  vtkMRMLPlusServerLauncherNode* GetLauncherNode();
  /// Set and observe launcher node
  void SetAndObserveLauncherNode(vtkMRMLPlusServerLauncherNode* node);

  void AddAndObservePlusOpenIGTLinkServerConnector(vtkMRMLIGTLConnectorNode* connectorNode);
  std::vector<vtkMRMLIGTLConnectorNode*> GetPlusOpenIGTLinkConnectorNodes();

  struct PlusOpenIGTLinkServerInfo
  {
    std::string OutputChannelId;
    int ListeningPort;
  };

  struct PlusConfigFileInfo
  {
    std::string Name;
    std::string Description;
    std::vector<vtkMRMLPlusServerNode::PlusOpenIGTLinkServerInfo> Servers;
  };

  static PlusConfigFileInfo GetPlusConfigFileInfo(std::string content);

  std::vector<PlusOpenIGTLinkServerInfo> GetPlusOpenIGTLinkServers() { return this->PlusOpenIGTLinkServers; };
  void SetPlusOpenIGTLinkServers(std::vector<PlusOpenIGTLinkServerInfo> plusOpenIGTLinkServers) { this->PlusOpenIGTLinkServers = plusOpenIGTLinkServers; };

protected:

  void UpdateConfigFileInfo();

  int DesiredState;
  int State;
  int LogLevel;
  std::string ConfigFileName;
  std::string DeviceSetName;
  std::string DeviceSetDescription;
  std::vector<PlusOpenIGTLinkServerInfo> PlusOpenIGTLinkServers;
};

#endif // __vtkMRMLPlusServerNode_h
