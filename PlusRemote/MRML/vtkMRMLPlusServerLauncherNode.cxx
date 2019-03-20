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

// PlusRemote includes
#include "vtkMRMLPlusServerLauncherNode.h"

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
const const char* vtkMRMLPlusServerLauncherNode::CONNECTOR_REFERENCE_ROLE = "connectorRef";
const const char* vtkMRMLPlusServerLauncherNode::PLUS_SERVER_REFERENCE_ROLE = "plusServerRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerLauncherNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode::vtkMRMLPlusServerLauncherNode()
  : Hostname("localhost")
  , Port(18904)
  , LogLevel(LogLevel::Info)
{
}

//----------------------------------------------------------------------------
vtkMRMLPlusServerLauncherNode::~vtkMRMLPlusServerLauncherNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStdStringMacro(Hostname, Hostname);
  vtkMRMLWriteXMLIntMacro(Port, Port);
  vtkMRMLWriteXMLIntMacro(LogLevel, LogLevel);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStdStringMacro(Hostname, Hostname);
  vtkMRMLReadXMLIntMacro(Port, Port);
  vtkMRMLReadXMLIntMacro(LogLevel, LogLevel);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyStdStringMacro(Hostname);
  vtkMRMLCopyIntMacro(Port);
  vtkMRMLCopyIntMacro(LogLevel);
  vtkMRMLCopyEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStdStringMacro(Hostname);
  vtkMRMLPrintIntMacro(Port);
  vtkMRMLPrintIntMacro(LogLevel);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLPlusServerLauncherNode::GetConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetNodeReference(CONNECTOR_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerLauncherNode::SetAndObserveConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  this->SetNodeReferenceID(CONNECTOR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
