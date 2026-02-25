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

#include "vtkIGTLToMRMLTransform.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioTransformDevice.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLTransform);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLTransform::GetMRMLNodeTagNames()
{
  return { "LinearTransform" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLTransform::CreateNode(vtkMRMLScene* scene,
                                                  igtlioDevice* device,
                                                  vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLTransformNode> transformNode =
    vtkMRMLTransformNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLTransformNode"));
  if (transformNode)
  {
    connector->RegisterIncomingMRMLNode(transformNode, device);
    return transformNode;
  }

  transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::Take(
    vtkMRMLLinearTransformNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLLinearTransformNode")));
  transformNode->SetName(deviceName.c_str());
  transformNode->SetDescription("Received by OpenIGTLink");
  vtkNew<vtkMatrix4x4> transform;
  transformNode->SetMatrixTransformToParent(transform);
  scene->AddNode(transformNode);
  connector->RegisterIncomingMRMLNode(transformNode, device);
  return transformNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLTransform::DeviceToMRMLInternal(igtlioDevice* device,
                                            vtkMRMLNode* node,
                                            vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioTransformDevice* transformDevice = reinterpret_cast<igtlioTransformDevice*>(device);
  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(node);
  if (!transformNode)
  {
    return;
  }
  vtkSmartPointer<vtkMatrix4x4> transformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  transformMatrix->DeepCopy(transformDevice->GetContent().transform);
  transformNode->SetMatrixTransformToParent(transformMatrix.GetPointer());
  transformNode->Modified();

  // Copy transform status from metadata to node attributes
  for (auto iter = device->GetMetaData().begin(); iter != device->GetMetaData().end(); ++iter)
  {
    if (iter->first.find("Status") != std::string::npos)
    {
      transformNode->SetAttribute(iter->first.c_str(), iter->second.second.c_str());
    }
  }
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLTransform::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioTransformDevice* transformDevice = static_cast<igtlioTransformDevice*>(device);
  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(node);
  vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
  transformNode->GetMatrixTransformToParent(mat);
  igtlioTransformConverter::ContentData content = { mat, transformNode->GetName(), "", "" };
  transformDevice->SetContent(content);
  return vtkMRMLTransformNode::TransformModifiedEvent;
}
