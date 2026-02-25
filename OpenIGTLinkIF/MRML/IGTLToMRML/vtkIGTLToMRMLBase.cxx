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

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University.

==============================================================================*/

#include "vtkIGTLToMRMLBase.h"

// OpenIGTLinkIO includes
#include <igtlioDevice.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObjectFactory.h>

const char* const vtkIGTLToMRMLBase::MRMLNodeNameKey = "MRMLNodeName";

//----------------------------------------------------------------------------
vtkIGTLToMRMLBase::vtkIGTLToMRMLBase()
{
}

//----------------------------------------------------------------------------
vtkIGTLToMRMLBase::~vtkIGTLToMRMLBase()
{
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IGTLDeviceType: " << this->GetIGTLDeviceType() << "\n";
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLBase::DeviceToMRML(igtlioDevice* device,
                                      vtkMRMLNode* node,
                                      vtkMRMLIGTLConnectorNode* connector)
{
  this->DeviceToMRMLInternal(device, node, connector);
  this->CopyMetaDataToMRMLNode(device, node);
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLBase::CopyMetaDataToMRMLNode(igtlioDevice* device, vtkMRMLNode* node)
{
  for (igtl::MessageBase::MetaDataMap::const_iterator iter = device->GetMetaData().begin();
       iter != device->GetMetaData().end(); ++iter)
  {
    std::string tag = "OpenIGTLink." + iter->first;
    node->SetAttribute(tag.c_str(), iter->second.second.c_str());
  }
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLBase::PrepareOutgoingDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  device->SetMetaDataElement(MRMLNodeNameKey, IANA_TYPE_US_ASCII, node->GetNodeTagName());
  // If no status is specified, the node is assumed to be valid.
  // Status should be set to "OK", rather than "UNKNOWN" to reflect this.
  const char* statusKey = this->GetStatusMetaDataKey();
  const char* status = node->GetAttribute(statusKey);
  device->SetMetaDataElement(statusKey, status ? status : "OK");
}
