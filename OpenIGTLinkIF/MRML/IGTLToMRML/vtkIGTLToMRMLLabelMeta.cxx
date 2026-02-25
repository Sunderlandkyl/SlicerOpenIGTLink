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

#include "vtkIGTLToMRMLLabelMeta.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLLabelMetaListNode.h"

// OpenIGTLinkIO includes
#include <igtlioLabelMetaDevice.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLLabelMeta);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLLabelMeta::GetMRMLNodeTagNames()
{
  return { "LabelMetaList" };
}

//----------------------------------------------------------------------------
static void CopyLabelMetaContent(igtlioLabelMetaDevice* labelMetaDevice,
                                   vtkMRMLLabelMetaListNode* labelMetaNode)
{
  labelMetaNode->ClearLabelMetaElement();
  igtlioLabelMetaConverter::LabelMetaDataList list = labelMetaDevice->GetContent().LabelMetaDataElements;
  for (auto it = list.begin(); it != list.end(); ++it)
  {
    vtkMRMLLabelMetaListNode::LabelMetaElement element;
    element.DeviceName = it->DeviceName;
    element.Label      = it->Label;
    element.Name       = it->Name;
    element.Owner      = it->Owner;
    for (int i = 0; i < 4; ++i) { element.RGBA[i] = it->RGBA[i]; }
    for (int i = 0; i < 3; ++i) { element.Size[i] = it->Size[i]; }
    labelMetaNode->AddLabelMetaElement(element);
  }
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLLabelMeta::CreateNode(vtkMRMLScene* scene,
                                                  igtlioDevice* device,
                                                  vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLLabelMetaListNode> labelMetaNode =
    vtkMRMLLabelMetaListNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLLabelMetaListNode"));
  if (labelMetaNode)
  {
    connector->RegisterIncomingMRMLNode(labelMetaNode, device);
    return labelMetaNode;
  }

  igtlioLabelMetaDevice* labelMetaDevice = reinterpret_cast<igtlioLabelMetaDevice*>(device);
  labelMetaNode = vtkSmartPointer<vtkMRMLLabelMetaListNode>::Take(
    vtkMRMLLabelMetaListNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLLabelMetaListNode")));
  labelMetaNode->SetName(deviceName.c_str());
  labelMetaNode->SetDescription("Received by OpenIGTLink");
  CopyLabelMetaContent(labelMetaDevice, labelMetaNode);
  scene->AddNode(labelMetaNode);
  connector->RegisterIncomingMRMLNode(labelMetaNode, device);
  return labelMetaNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLLabelMeta::DeviceToMRMLInternal(igtlioDevice* device,
                                            vtkMRMLNode* node,
                                            vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioLabelMetaDevice* labelMetaDevice = reinterpret_cast<igtlioLabelMetaDevice*>(device);
  vtkMRMLLabelMetaListNode* labelMetaNode = vtkMRMLLabelMetaListNode::SafeDownCast(node);
  if (!labelMetaNode)
  {
    return;
  }
  CopyLabelMetaContent(labelMetaDevice, labelMetaNode);
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLLabelMeta::MRMLToDevice(vtkMRMLNode* vtkNotUsed(node),
                                                    igtlioDevice* vtkNotUsed(device))
{
  // LBMETA is a query/response type — outgoing push is not supported.
  return 0;
}
