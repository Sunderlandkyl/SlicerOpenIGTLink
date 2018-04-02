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

#ifndef __vtkMRMLTextStorageNode_h
#define __vtkMRMLTextStorageNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// Slicer includes
#include "vtkMRMLStorageNode.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLTextStorageNode : public vtkMRMLStorageNode
{
public:
  static vtkMRMLTextStorageNode *New();
  vtkTypeMacro(vtkMRMLTextStorageNode,vtkMRMLStorageNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "TextStorage";}

  /// Return true if the node can be read in
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode) VTK_OVERRIDE;

protected:
  vtkMRMLTextStorageNode();
  ~vtkMRMLTextStorageNode();
  vtkMRMLTextStorageNode(const vtkMRMLTextStorageNode&);
  void operator=(const vtkMRMLTextStorageNode&);

  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes() VTK_OVERRIDE;

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes() VTK_OVERRIDE;

  /// Read data and set it in the referenced node
  virtual int ReadDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

  /// Write data from a  referenced node
  virtual int WriteDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

};

#endif



