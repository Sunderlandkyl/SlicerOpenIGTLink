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

#include "vtkIGTLToMRMLImage.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioImageDevice.h>

// MRML includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLImage);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLImage::GetMRMLNodeTagNames()
{
  return { "Volume", "VectorVolume", "StreamingVolume" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLImage::CreateNode(vtkMRMLScene* scene,
                                              igtlioDevice* device,
                                              vtkMRMLIGTLConnectorNode* connector)
{
  igtlioImageDevice* imageDevice = reinterpret_cast<igtlioImageDevice*>(device);
  igtlioImageConverter::ContentData content = imageDevice->GetContent();
  if (!content.image)
  {
    return nullptr;
  }

  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();
  int numberOfComponents = content.image->GetNumberOfScalarComponents();

  vtkSmartPointer<vtkMRMLVolumeNode> volumeNode;

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  if (connector->GetUseStreamingVolume())
  {
    volumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(
      scene->GetFirstNode(deviceName.c_str(), "vtkMRMLStreamingVolumeNode"));
    if (volumeNode)
    {
      connector->RegisterIncomingMRMLNode(volumeNode, device);
      return volumeNode;
    }
    volumeNode = vtkSmartPointer<vtkMRMLVolumeNode>::Take(
      vtkMRMLVolumeNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLStreamingVolumeNode")));
  }
  else
#endif
  if (numberOfComponents > 1)
  {
    volumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(
      scene->GetFirstNode(deviceName.c_str(), "vtkMRMLVectorVolumeNode"));
    if (volumeNode)
    {
      connector->RegisterIncomingMRMLNode(volumeNode, device);
      return volumeNode;
    }
    volumeNode = vtkSmartPointer<vtkMRMLVolumeNode>::Take(
      vtkMRMLVolumeNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLVectorVolumeNode")));
  }
  else
  {
    volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
      scene->GetFirstNode(deviceName.c_str(), "vtkMRMLScalarVolumeNode"));
    if (volumeNode)
    {
      connector->RegisterIncomingMRMLNode(volumeNode, device);
      return volumeNode;
    }
    volumeNode = vtkSmartPointer<vtkMRMLVolumeNode>::Take(
      vtkMRMLVolumeNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLScalarVolumeNode")));
  }

  volumeNode->SetIJKToRASMatrix(content.transform);
  volumeNode->SetAndObserveImageData(content.image);
  volumeNode->SetName(deviceName.c_str());
  scene->SaveStateForUndo();
  volumeNode->SetDescription("Received by OpenIGTLink");
  scene->AddNode(volumeNode);

  bool scalarDisplayNodeRequired = (numberOfComponents == 1);
  vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
  if (scalarDisplayNodeRequired)
  {
    displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::Take(
      vtkMRMLScalarVolumeDisplayNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLScalarVolumeDisplayNode")));
  }
  else
  {
    displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::Take(
      vtkMRMLVectorVolumeDisplayNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLVectorVolumeDisplayNode")));
  }
  scene->AddNode(displayNode);

  if (scalarDisplayNodeRequired)
  {
    const char* colorTableId = vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
    displayNode->SetAndObserveColorNodeID(colorTableId);
  }
  else
  {
    displayNode->SetDefaultColorMap();
  }

  volumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  connector->RegisterIncomingMRMLNode(volumeNode, device);
  return volumeNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLImage::DeviceToMRML(igtlioDevice* device,
                                       vtkMRMLNode* node,
                                       vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioImageDevice* imageDevice = reinterpret_cast<igtlioImageDevice*>(device);
  vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(node);
  if (!volumeNode)
  {
    return;
  }
  volumeNode->SetIJKToRASMatrix(imageDevice->GetContent().transform);
  volumeNode->SetAndObserveImageData(imageDevice->GetContent().image);
  volumeNode->GetImageData()->SetSpacing(1.0, 1.0, 1.0);
  volumeNode->GetImageData()->SetOrigin(0.0, 0.0, 0.0);
#if VTK_MAJOR_VERSION >= 9
  vtkSmartPointer<vtkMatrix3x3> mat = vtkSmartPointer<vtkMatrix3x3>::New();
  mat->Identity();
  volumeNode->GetImageData()->SetDirectionMatrix(mat);
#endif
  volumeNode->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLImage::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioImageDevice* imageDevice = static_cast<igtlioImageDevice*>(device);
  vtkMRMLVolumeNode* imageNode = vtkMRMLVolumeNode::SafeDownCast(node);
  vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
  imageNode->GetIJKToRASMatrix(mat);
  igtlioImageConverter::ContentData content = { imageNode->GetImageData(), mat };
  imageDevice->SetContent(content);
  return vtkMRMLVolumeNode::ImageDataModifiedEvent;
}
