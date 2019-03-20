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
#include "vtkMRMLPlusServerNode.h"

// PlusRemote includes
#include "vtkMRMLPlusServerLauncherNode.h"


// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

// OpenIGTLinkIF includes
#include <vtkMRMLIGTLConnectorNode.h>
#include <vtkMRMLTextNode.h>
#include <vtkMRMLNode.h>

//------------------------------------------------------------------------------
static const char* CONFIG_REFERENCE_ROLE = "configNodeRef";
static const char* LAUNCHER_REFERENCE_ROLE = "launcherNodeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusServerNode);

//----------------------------------------------------------------------------
vtkMRMLPlusServerNode::vtkMRMLPlusServerNode()
  : DesiredState(ServerStatus::Off)
  , State(ServerStatus::Off)
  , LogLevel(LogLevel::Info)
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
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::Copy(vtkMRMLNode* anode)
{
  Superclass::Copy(anode);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
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
}

//----------------------------------------------------------------------------
vtkMRMLTextNode* vtkMRMLPlusServerNode::GetConfigNode()
{
  return vtkMRMLTextNode::SafeDownCast(this->GetNodeReference(CONFIG_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusServerNode::SetAndObserveConfigNode(vtkMRMLTextNode* node)
{
  this->SetNodeReferenceID(CONFIG_REFERENCE_ROLE, (node ? node->GetID() : NULL));
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
