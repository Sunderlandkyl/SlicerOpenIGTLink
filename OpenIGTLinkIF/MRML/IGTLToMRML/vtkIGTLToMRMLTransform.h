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
#ifndef __vtkIGTLToMRMLTransform_h
#define __vtkIGTLToMRMLTransform_h

#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLBase.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLTransform : public vtkIGTLToMRMLBase
{
public:
  static vtkIGTLToMRMLTransform* New();
  vtkTypeMacro(vtkIGTLToMRMLTransform, vtkIGTLToMRMLBase);

  std::string GetIGTLDeviceType() const override { return "TRANSFORM"; }
  std::vector<std::string> GetMRMLNodeTagNames() override;
  vtkMRMLNode* CreateNode(vtkMRMLScene* scene, igtlioDevice* device, vtkMRMLIGTLConnectorNode* connector) override;
  void DeviceToMRMLInternal(igtlioDevice* device, vtkMRMLNode* node, vtkMRMLIGTLConnectorNode* connector) override;
  unsigned int MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device) override;
  const char* GetStatusMetaDataKey() const override { return "TransformStatus"; }

protected:
  vtkIGTLToMRMLTransform() = default;
  ~vtkIGTLToMRMLTransform() override = default;

private:
  vtkIGTLToMRMLTransform(const vtkIGTLToMRMLTransform&) = delete;
  void operator=(const vtkIGTLToMRMLTransform&) = delete;
};

#endif
