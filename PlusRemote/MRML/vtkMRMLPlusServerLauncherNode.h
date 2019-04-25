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

#ifndef __vtkMRMLPlusServerLauncherNode_h
#define __vtkMRMLPlusServerLauncherNode_h

// MRML includes
#include <vtkMRMLNode.h>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"


// VTK includes
#include <vtkCommand.h>

class vtkMRMLIGTLConnectorNode;
class vtkMRMLPlusServerNode;
class vtkMRMLScene;
class vtkMRMLTextNode;

/// \ingroup IGT
/// \brief Parameter set node for the plus remote launcher widget
///
class VTK_SLICER_PLUSREMOTE_MODULE_MRML_EXPORT vtkMRMLPlusServerLauncherNode : public vtkMRMLNode
{
public:
  static vtkMRMLPlusServerLauncherNode* New();
  vtkTypeMacro(vtkMRMLPlusServerLauncherNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Standard MRML node methods
  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(ostream& of, int indent) override;
  virtual void Copy(vtkMRMLNode* node) override;
  virtual const char* GetNodeTagName() override { return "PlusServerLauncher"; }

protected:
  vtkMRMLPlusServerLauncherNode();
  virtual ~vtkMRMLPlusServerLauncherNode();
  vtkMRMLPlusServerLauncherNode(const vtkMRMLPlusServerLauncherNode&);
  void operator=(const vtkMRMLPlusServerLauncherNode&);

public:
  static const char* CONNECTOR_REFERENCE_ROLE;
  static const char* PLUS_SERVER_REFERENCE_ROLE;

  enum
  {
    LOG_ERROR = 1,
    LOG_WARNING = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_TRACE = 5,
  };

  enum
  {
    ServerAddedEvent = vtkCommand::UserEvent + 701,
    ServerRemovedEvent,
  };

  vtkGetMacro(Hostname, std::string);
  vtkSetMacro(Hostname, std::string);

  vtkGetMacro(Port, int);
  vtkSetMacro(Port, int);

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);

  // Create a server node using the configFileNode and set the desired state to on
  vtkMRMLPlusServerNode* StartServer(vtkMRMLTextNode* configFileNode, int logLevel = LOG_INFO);

  /// Get launcher connector node
  vtkMRMLIGTLConnectorNode* GetConnectorNode();
  /// Set and observe launcher connector node
  void SetAndObserveConnectorNode(vtkMRMLIGTLConnectorNode* node);
  ///
  void AddAndObserveServerNode(vtkMRMLPlusServerNode* serverNode);
  ///
  void RemoveServerNode(vtkMRMLPlusServerNode* serverNode);
  ///
  std::vector<vtkMRMLPlusServerNode*> GetServerNodes();

private:
  std::string Hostname;
  int Port;
  int LogLevel;
};

#endif // __vtkMRMLPlusServerLauncherNode_h
