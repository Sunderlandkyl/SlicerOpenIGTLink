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
#ifndef __vtkIGTLToMRMLImageMeta_h
#define __vtkIGTLToMRMLImageMeta_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLBase.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLImageMeta : public vtkIGTLToMRMLBase
{
public:
  static vtkIGTLToMRMLImageMeta* New();
  vtkTypeMacro(vtkIGTLToMRMLImageMeta, vtkIGTLToMRMLBase);

  std::string GetIGTLDeviceType() const override { return "IMGMETA"; }
  std::vector<std::string> GetMRMLNodeTagNames() override;
  vtkMRMLNode* CreateNode(vtkMRMLScene* scene, igtlioDevice* device, vtkMRMLIGTLConnectorNode* connector) override;
  void DeviceToMRMLInternal(igtlioDevice* device, vtkMRMLNode* node, vtkMRMLIGTLConnectorNode* connector) override;
  unsigned int MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device) override;

protected:
  vtkIGTLToMRMLImageMeta() = default;
  ~vtkIGTLToMRMLImageMeta() override = default;

private:
  vtkIGTLToMRMLImageMeta(const vtkIGTLToMRMLImageMeta&) = delete;
  void operator=(const vtkIGTLToMRMLImageMeta&) = delete;
};

#endif
