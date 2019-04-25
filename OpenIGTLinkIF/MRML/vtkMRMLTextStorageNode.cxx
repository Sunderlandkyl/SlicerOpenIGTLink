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

// STD includes
#include <algorithm>
#include <sstream>

#include <vtkMRMLTextNode.h>
#include "vtkMRMLTextStorageNode.h"

// MRML includes
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkNew.h>
#include <vtkStringArray.h>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTextStorageNode);

//----------------------------------------------------------------------------
vtkMRMLTextStorageNode::vtkMRMLTextStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLTextStorageNode::~vtkMRMLTextStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLTextStorageNode::CanReadInReferenceNode(vtkMRMLNode* refNode)
{
  return refNode->IsA("vtkMRMLTextNode");
}

//----------------------------------------------------------------------------
int vtkMRMLTextStorageNode::ReadDataInternal(vtkMRMLNode * refNode)
{
  if (!this->CanReadInReferenceNode(refNode))
  {
    return 0;
  }

  vtkMRMLTextNode* textNode = dynamic_cast<vtkMRMLTextNode*>(refNode);
  if (!textNode)
  {
    vtkErrorMacro("ReadDataInternal: not a text node.");
    return 0;
  }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadData: File name not specified");
    return 0;
  }

  // success
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLTextStorageNode::CanWriteFromReferenceNode(vtkMRMLNode * refNode)
{
  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(refNode);
  if (textNode == NULL)
  {
    vtkErrorMacro("vtkMRMLTextStorageNode::CanWriteFromReferenceNode: input is not a text node");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLTextStorageNode::WriteDataInternal(vtkMRMLNode * refNode)
{
  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(refNode);
  if (textNode == NULL)
  {
    vtkErrorMacro(<< "vtkMRMLTextStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
  }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("WriteData: File name not specified");
    return 0;
  }

  this->StageWriteData(refNode);
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLTextStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Text node (.txt)");
  this->SupportedReadFileTypes->InsertNextValue("XML document (.xml)");
  this->SupportedReadFileTypes->InsertNextValue("JSON document (.json)");
}

//----------------------------------------------------------------------------
void vtkMRMLTextStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Text node (.txt)");
  this->SupportedReadFileTypes->InsertNextValue("XML document (.xml)");
  this->SupportedReadFileTypes->InsertNextValue("JSON document (.json)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLTextStorageNode::GetDefaultWriteFileExtension()
{
  return "txt";
}