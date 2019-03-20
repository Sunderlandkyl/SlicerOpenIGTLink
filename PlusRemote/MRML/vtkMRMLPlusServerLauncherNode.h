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

#ifndef __vtkMRMLPlusServerLauncherNode_h
#define __vtkMRMLPlusServerLauncherNode_h

// MRML includes
#include <vtkMRMLNode.h>

// PlusRemote includes
#include "vtkSlicerPlusRemoteModuleMRMLExport.h"

class vtkMRMLScene;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLTextNode;

/// \ingroup Segmentations
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

  enum LogLevel
  {
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
  };

  vtkGetMacro(State, int);
  vtkSetMacro(State, int);

  vtkGetMacro(Hostname, std::string);
  vtkSetMacro(Hostname, std::string);

  vtkGetMacro(Port, int);
  vtkSetMacro(Port, int);

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);

  /// Get launcher connector node
  vtkMRMLIGTLConnectorNode* GetConnectorNode();
  /// Set and observe launcher connector node
  void SetAndObserveConnectorNode(vtkMRMLIGTLConnectorNode* node);

private:
  int State;
  std::string Hostname;
  int Port;
  int LogLevel;
};

#endif // __vtkMRMLPlusServerLauncherNode_h
