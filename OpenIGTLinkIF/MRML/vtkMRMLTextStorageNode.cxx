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

// OpenIGTLinkIF includes
#include "vtkMRMLTextStorageNode.h"
#include "vtkMRMLTextNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTextStorageNode);

//----------------------------------------------------------------------------
vtkMRMLTextStorageNode::vtkMRMLTextStorageNode()
{
  this->DefaultWriteFileExtension = "txt";
}

//----------------------------------------------------------------------------
vtkMRMLTextStorageNode::~vtkMRMLTextStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLTextStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMRMLTextStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLTextNode");
}

//----------------------------------------------------------------------------
int vtkMRMLTextStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  std::string fullName = this->GetFullNameFromFileName();

  if (fullName.empty())
    {
    vtkErrorMacro("vtkMRMLTextStorageNode: File name not specified");
    return 0;
    }

  // cast the input node
  vtkMRMLTextNode *textNode =
    vtkMRMLTextNode::SafeDownCast(refNode);

  std::string s(std::istreambuf_iterator<char>(std::ifstream(fullName)
    >> std::skipws),
    std::istreambuf_iterator<char>());
  textNode->SetText(s.c_str());

  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLTextStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
    if (this->GetFileName() == NULL)
    {
        vtkErrorMacro("WriteData: file name is not set");
        return 0;
    }

    std::string fullName = this->GetFullNameFromFileName();
    if (fullName.empty())
    {
        vtkErrorMacro("vtkMRMLTextStorageNode: File name not specified");
        return 0;
    }

    // cast the input node
    vtkMRMLTextNode *textNode =
      vtkMRMLTextNode::SafeDownCast(textNode);

    return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLTextStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Text (.txt)");
  this->SupportedReadFileTypes->InsertNextValue("XML (.xml)");
}

//----------------------------------------------------------------------------
void vtkMRMLTextStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Text (.txt)");
  this->SupportedWriteFileTypes->InsertNextValue("XML (.xml)");
}
