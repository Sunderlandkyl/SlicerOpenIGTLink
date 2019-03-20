/*==============================================================================

  Program: 3D Slicer

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

#ifndef __vtkMRMLPlusServerNode_h
#define __vtkMRMLPlusServerNode_h

// MRML includes
#include <vtkMRMLNode.h>

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

  static vtkMRMLPlusServerNode* New();
  vtkTypeMacro(vtkMRMLPlusServerNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Standard MRML node methods
  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual void ReadXMLAttributes(const char** atts) override;
  virtual void WriteXML(ostream& of, int indent) override;
  virtual void Copy(vtkMRMLNode* node) override;
  virtual const char* GetNodeTagName() override { return "PlusServer"; }

protected:
  vtkMRMLPlusServerNode();
  virtual ~vtkMRMLPlusServerNode();
  vtkMRMLPlusServerNode(const vtkMRMLPlusServerNode&);
  void operator=(const vtkMRMLPlusServerNode&);

public:

  enum LogLevel
  {
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
  };

  vtkGetMacro(DesiredState, int);
  vtkSetMacro(DesiredState, int);

  vtkGetMacro(State, int);
  vtkSetMacro(State, int);

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);

  vtkGetMacro(ConfigFileName, std::string);
  vtkSetMacro(ConfigFileName, std::string);

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

protected:
  int DesiredState;
  int State;
  int LogLevel;
  std::string ConfigFileName;
};

#endif // __vtkMRMLPlusServerNode_h
