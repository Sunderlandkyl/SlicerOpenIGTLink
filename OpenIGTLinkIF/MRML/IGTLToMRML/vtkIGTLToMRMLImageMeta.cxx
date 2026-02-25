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

#include "vtkIGTLToMRMLImageMeta.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLImageMetaListNode.h"
#include "vtkMRMLImageMetaElement.h"

// OpenIGTLinkIO includes
#include <igtlioImageMetaDevice.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLImageMeta);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLImageMeta::GetMRMLNodeTagNames()
{
  return { "ImageMetaList" };
}

//----------------------------------------------------------------------------
static void CopyImageMetaContent(igtlioImageMetaDevice* imageMetaDevice,
                                  vtkMRMLImageMetaListNode* imageMetaNode)
{
  imageMetaNode->ClearImageMetaElement();
  igtlioImageMetaConverter::ImageMetaDataList list = imageMetaDevice->GetContent().ImageMetaDataElements;
  for (auto it = list.begin(); it != list.end(); ++it)
  {
    vtkMRMLImageMetaElement element;
    element.DeviceName   = it->DeviceName;
    element.Modality     = it->Modality;
    element.Name         = it->Name;
    element.PatientID    = it->PatientID;
    element.PatientName  = it->PatientName;
    element.ScalarType   = it->ScalarType;
    for (int i = 0; i < 3; ++i) { element.Size[i] = it->Size[i]; }
    element.TimeStamp    = it->Timestamp;
    imageMetaNode->AddImageMetaElement(element);
  }
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLImageMeta::CreateNode(vtkMRMLScene* scene,
                                                  igtlioDevice* device,
                                                  vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLImageMetaListNode> imageMetaNode =
    vtkMRMLImageMetaListNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLImageMetaListNode"));
  if (imageMetaNode)
  {
    connector->RegisterIncomingMRMLNode(imageMetaNode, device);
    return imageMetaNode;
  }

  igtlioImageMetaDevice* imageMetaDevice = reinterpret_cast<igtlioImageMetaDevice*>(device);
  imageMetaNode = vtkSmartPointer<vtkMRMLImageMetaListNode>::Take(
    vtkMRMLImageMetaListNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLImageMetaListNode")));
  imageMetaNode->SetName(deviceName.c_str());
  imageMetaNode->SetDescription("Received by OpenIGTLink");
  CopyImageMetaContent(imageMetaDevice, imageMetaNode);
  scene->AddNode(imageMetaNode);
  connector->RegisterIncomingMRMLNode(imageMetaNode, device);
  return imageMetaNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLImageMeta::DeviceToMRMLInternal(igtlioDevice* device,
                                            vtkMRMLNode* node,
                                            vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioImageMetaDevice* imageMetaDevice = reinterpret_cast<igtlioImageMetaDevice*>(device);
  vtkMRMLImageMetaListNode* imageMetaNode = vtkMRMLImageMetaListNode::SafeDownCast(node);
  if (!imageMetaNode)
  {
    return;
  }
  CopyImageMetaContent(imageMetaDevice, imageMetaNode);
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLImageMeta::MRMLToDevice(vtkMRMLNode* vtkNotUsed(node),
                                                    igtlioDevice* vtkNotUsed(device))
{
  // IMGMETA is a query/response type — outgoing push is not supported.
  return 0;
}
