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

#include "vtkIGTLToMRMLStatus.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLStatusNode.h"

// OpenIGTLinkIO includes
#include <igtlioStatusDevice.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLStatus);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLStatus::GetMRMLNodeTagNames()
{
  return { "IGTLStatus" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLStatus::CreateNode(vtkMRMLScene* scene,
                                               igtlioDevice* device,
                                               vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLIGTLStatusNode> statusNode =
    vtkMRMLIGTLStatusNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLIGTLStatusNode"));
  if (statusNode)
  {
    connector->RegisterIncomingMRMLNode(statusNode, device);
    return statusNode;
  }

  statusNode = vtkSmartPointer<vtkMRMLIGTLStatusNode>::Take(
    vtkMRMLIGTLStatusNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLIGTLStatusNode")));
  statusNode->SetName(deviceName.c_str());
  statusNode->SetDescription("Received by OpenIGTLink");
  scene->AddNode(statusNode);
  connector->RegisterIncomingMRMLNode(statusNode, device);
  return statusNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLStatus::DeviceToMRMLInternal(igtlioDevice* device,
                                         vtkMRMLNode* node,
                                         vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioStatusDevice* statusDevice = reinterpret_cast<igtlioStatusDevice*>(device);
  vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
  if (!statusNode)
  {
    return;
  }
  statusNode->SetStatus(statusDevice->GetContent().code,
                         statusDevice->GetContent().subcode,
                         statusDevice->GetContent().errorname.c_str(),
                         statusDevice->GetContent().statusstring.c_str());
  statusNode->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLStatus::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioStatusDevice* statusDevice = static_cast<igtlioStatusDevice*>(device);
  vtkMRMLIGTLStatusNode* statusNode = vtkMRMLIGTLStatusNode::SafeDownCast(node);
  igtlioStatusConverter::ContentData content = {
    static_cast<int>(statusNode->GetCode()),
    static_cast<int>(statusNode->GetSubCode()),
    statusNode->GetErrorName(),
    statusNode->GetStatusString()
  };
  statusDevice->SetContent(content);
  return vtkMRMLIGTLStatusNode::StatusModifiedEvent;
}
