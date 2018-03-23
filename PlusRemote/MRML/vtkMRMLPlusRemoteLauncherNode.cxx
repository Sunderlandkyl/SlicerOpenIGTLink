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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// PlusRemote includes
#include "vtkMRMLPlusRemoteLauncherNode.h"

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
static const char* LAUNCHER_CONNECTOR_REFERENCE_ROLE = "launcherConnectorRef";
static const char* CURRENT_CONFIG_REFERENCE_ROLE = "currentConfigRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPlusRemoteLauncherNode);

//----------------------------------------------------------------------------
vtkMRMLPlusRemoteLauncherNode::vtkMRMLPlusRemoteLauncherNode()
  : Hostname("localhost")
  , ServerState(vtkMRMLPlusRemoteLauncherNode::ServerOff) //TODO
  , ServerLauncherPort(vtkMRMLPlusRemoteLauncherNode::DefaultPort)
{
}

//----------------------------------------------------------------------------
vtkMRMLPlusRemoteLauncherNode::~vtkMRMLPlusRemoteLauncherNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  of << " hostname=\"" << this->Hostname << "\"";
  of << " serverLauncherPort =\"" << this->ServerLauncherPort << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "hostname"))
      {
      this->SetHostname(attValue);
      }
    else if (!strcmp(attName, "serverLauncherPort"))
      {
      vtkVariant portVariant = attValue;
      portVariant.ToInt();
      int portValue = portValue = portVariant.ToInt();
      if (!portVariant.IsValid())
      {
        portValue = vtkMRMLPlusRemoteLauncherNode::DefaultPort;
      }
      this->SetServerLauncherPort(portValue);
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLPlusRemoteLauncherNode* otherNode = vtkMRMLPlusRemoteLauncherNode::SafeDownCast(anode);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkMRMLPlusRemoteLauncherNode::GetLauncherConnectorNode()
{
  return vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetNodeReference(LAUNCHER_CONNECTOR_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::SetAndObserveLauncherConnectorNode(vtkMRMLIGTLConnectorNode* node)
{
  this->SetNodeReferenceID(LAUNCHER_CONNECTOR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLTextNode* vtkMRMLPlusRemoteLauncherNode::GetCurrentConfigNode()
{
  return vtkMRMLTextNode::SafeDownCast(this->GetNodeReference(CURRENT_CONFIG_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLPlusRemoteLauncherNode::SetAndObserveCurrentConfigNode(vtkMRMLTextNode* node)
{
  this->SetNodeReferenceID(CURRENT_CONFIG_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
