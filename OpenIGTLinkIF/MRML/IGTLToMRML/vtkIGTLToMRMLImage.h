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
#ifndef __vtkIGTLToMRMLImage_h
#define __vtkIGTLToMRMLImage_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLBase.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLImage : public vtkIGTLToMRMLBase
{
public:
  static vtkIGTLToMRMLImage* New();
  vtkTypeMacro(vtkIGTLToMRMLImage, vtkIGTLToMRMLBase);

  std::string GetIGTLDeviceType() const override { return "IMAGE"; }
  std::vector<std::string> GetMRMLNodeTagNames() override;
  vtkMRMLNode* CreateNode(vtkMRMLScene* scene, igtlioDevice* device, vtkMRMLIGTLConnectorNode* connector) override;
  void DeviceToMRMLInternal(igtlioDevice* device, vtkMRMLNode* node, vtkMRMLIGTLConnectorNode* connector) override;
  unsigned int MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device) override;

protected:
  vtkIGTLToMRMLImage() = default;
  ~vtkIGTLToMRMLImage() override = default;

private:
  vtkIGTLToMRMLImage(const vtkIGTLToMRMLImage&) = delete;
  void operator=(const vtkIGTLToMRMLImage&) = delete;
};

#endif
