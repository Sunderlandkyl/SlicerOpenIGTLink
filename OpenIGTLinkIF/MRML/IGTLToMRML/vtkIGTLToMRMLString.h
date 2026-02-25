/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University.

==============================================================================*/
#ifndef __vtkIGTLToMRMLString_h
#define __vtkIGTLToMRMLString_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLBase.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLString : public vtkIGTLToMRMLBase
{
public:
  static vtkIGTLToMRMLString* New();
  vtkTypeMacro(vtkIGTLToMRMLString, vtkIGTLToMRMLBase);

  std::string GetIGTLDeviceType() const override { return "STRING"; }
  std::vector<std::string> GetMRMLNodeTagNames() override;
  vtkMRMLNode* CreateNode(vtkMRMLScene* scene, igtlioDevice* device, vtkMRMLIGTLConnectorNode* connector) override;
  void DeviceToMRMLInternal(igtlioDevice* device, vtkMRMLNode* node, vtkMRMLIGTLConnectorNode* connector) override;
  unsigned int MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device) override;

protected:
  vtkIGTLToMRMLString() = default;
  ~vtkIGTLToMRMLString() override = default;

private:
  vtkIGTLToMRMLString(const vtkIGTLToMRMLString&) = delete;
  void operator=(const vtkIGTLToMRMLString&) = delete;
};

#endif
