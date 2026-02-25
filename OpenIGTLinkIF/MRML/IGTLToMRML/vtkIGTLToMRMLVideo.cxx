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

#include "vtkIGTLToMRMLVideo.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioVideoDevice.h>

// MRML includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLVolumeDisplayNode.h>

// vtkAddon includes
#include <vtkStreamingVolumeFrame.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkUnsignedCharArray.h>

vtkStandardNewMacro(vtkIGTLToMRMLVideo);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLVideo::GetMRMLNodeTagNames()
{
  return { "StreamingVolume" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLVideo::CreateNode(vtkMRMLScene* scene,
                                              igtlioDevice* device,
                                              vtkMRMLIGTLConnectorNode* connector)
{
  igtlioVideoDevice* videoDevice = reinterpret_cast<igtlioVideoDevice*>(device);
  igtlioVideoConverter::ContentData content = videoDevice->GetContent();
  if (!content.frameData)
  {
    return nullptr;
  }
  int numberOfComponents = content.grayscale ? 1 : 3;
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLStreamingVolumeNode> streamingVolumeNode =
    vtkMRMLStreamingVolumeNode::SafeDownCast(
      scene->GetFirstNode(deviceName.c_str(), "vtkMRMLStreamingVolumeNode"));
  if (streamingVolumeNode)
  {
    connector->RegisterIncomingMRMLNode(streamingVolumeNode, device);
    return streamingVolumeNode;
  }

  scene->SaveStateForUndo();
  bool scalarDisplayNodeRequired = (numberOfComponents == 1);
  streamingVolumeNode = vtkSmartPointer<vtkMRMLStreamingVolumeNode>::Take(
    vtkMRMLStreamingVolumeNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLStreamingVolumeNode")));
  streamingVolumeNode->SetName(deviceName.c_str());
  streamingVolumeNode->SetDescription("Received by OpenIGTLink");
  scene->AddNode(streamingVolumeNode);

  vtkSmartPointer<vtkMRMLVolumeDisplayNode> displayNode;
  if (scalarDisplayNodeRequired)
  {
    displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::Take(
      vtkMRMLScalarVolumeDisplayNode::SafeDownCast(
        scene->CreateNodeByClass("vtkMRMLScalarVolumeDisplayNode")));
  }
  else
  {
    displayNode = vtkSmartPointer<vtkMRMLVectorVolumeDisplayNode>::Take(
      vtkMRMLVectorVolumeDisplayNode::SafeDownCast(
        scene->CreateNodeByClass("vtkMRMLVectorVolumeDisplayNode")));
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
  streamingVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  connector->RegisterIncomingMRMLNode(streamingVolumeNode, device);
  return streamingVolumeNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLVideo::DeviceToMRMLInternal(igtlioDevice* device,
                                        vtkMRMLNode* node,
                                        vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioVideoDevice* videoDevice = reinterpret_cast<igtlioVideoDevice*>(device);
  vtkMRMLStreamingVolumeNode* streamingVolumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(node);
  if (!streamingVolumeNode)
  {
    return;
  }

  vtkSmartPointer<vtkUnsignedCharArray> frameData = vtkSmartPointer<vtkUnsignedCharArray>::New();
  frameData->Allocate(videoDevice->GetContent().videoMessage->GetBitStreamSize());
  memcpy(frameData->GetPointer(0),
         videoDevice->GetContent().frameData->GetPointer(0),
         videoDevice->GetContent().videoMessage->GetBitStreamSize());

  std::string codecName = videoDevice->GetCurrentCodecType().substr(0, 4);
  vtkSmartPointer<vtkStreamingVolumeFrame> frame = vtkSmartPointer<vtkStreamingVolumeFrame>::New();
  frame->SetFrameData(frameData);
  frame->SetFrameType(videoDevice->GetContent().frameType == igtl::FrameTypeKey
                        ? vtkStreamingVolumeFrame::IFrame
                        : vtkStreamingVolumeFrame::PFrame);
  videoDevice->GetContent().videoMessage->Unpack(false);
  frame->SetDimensions(videoDevice->GetContent().videoMessage->GetWidth(),
                        videoDevice->GetContent().videoMessage->GetHeight(),
                        videoDevice->GetContent().videoMessage->GetAdditionalZDimension());
  frame->SetNumberOfComponents(videoDevice->GetContent().grayscale ? 1 : 3);
  frame->SetCodecFourCC(codecName);

  if (!frame->IsKeyFrame())
  {
    // A non-key frame can only be decoded relative to the frame received before it.
    // That frame is the one currently held by the node, so no separate bookkeeping is needed.
    frame->SetPreviousFrame(streamingVolumeNode->GetFrame());
  }

  streamingVolumeNode->SetIJKToRASMatrix(videoDevice->GetContent().transform);
  streamingVolumeNode->SetAndObserveFrame(frame);
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLVideo::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioVideoDevice* videoDevice = static_cast<igtlioVideoDevice*>(device);
  vtkMRMLStreamingVolumeNode* streamingVolumeNode = vtkMRMLStreamingVolumeNode::SafeDownCast(node);
  igtlioVideoConverter::ContentData content;
  content.image = streamingVolumeNode->GetImageData();
  content.frameType = FrameTypeUnKnown;
  strncpy(content.codecName, videoDevice->GetCurrentCodecType().c_str(), IGTL_VIDEO_CODEC_NAME_SIZE);
  content.keyFrameMessage = nullptr;
  content.keyFrameUpdated = false;
  content.videoMessage = nullptr;
  videoDevice->SetContent(content);
  return vtkCommand::ModifiedEvent;
}
