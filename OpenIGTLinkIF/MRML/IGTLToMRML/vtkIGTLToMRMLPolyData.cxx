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

#include "vtkIGTLToMRMLPolyData.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioPolyDataDevice.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

vtkStandardNewMacro(vtkIGTLToMRMLPolyData);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLPolyData::GetMRMLNodeTagNames()
{
  return { "Model", "FiberBundle" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLPolyData::CreateNode(vtkMRMLScene* scene,
                                                 igtlioDevice* device,
                                                 vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLModelNode> modelNode =
    vtkMRMLModelNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLModelNode"));
  if (modelNode)
  {
    connector->RegisterIncomingMRMLNode(modelNode, device);
    return modelNode;
  }

  igtlioPolyDataDevice* polyDevice = reinterpret_cast<igtlioPolyDataDevice*>(device);
  igtlioPolyDataConverter::ContentData content = polyDevice->GetContent();

  std::string mrmlNodeTagName;
  if (device->GetMetaDataElement(MRMLNodeNameKey, mrmlNodeTagName))
  {
    std::string className = scene->GetClassNameByTag(mrmlNodeTagName.c_str());
    vtkMRMLNode* createdNode = scene->CreateNodeByClass(className.c_str());
    if (createdNode)
    {
      modelNode = vtkMRMLModelNode::SafeDownCast(createdNode);
    }
  }

  if (!modelNode)
  {
    modelNode = vtkSmartPointer<vtkMRMLModelNode>::Take(
      vtkMRMLModelNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLModelNode")));
  }

  modelNode->SetName(deviceName.c_str());
  modelNode->SetDescription("Received by OpenIGTLink");
  scene->AddNode(modelNode);
  modelNode->SetAndObservePolyData(content.polydata);
  modelNode->CreateDefaultDisplayNodes();
  connector->RegisterIncomingMRMLNode(modelNode, device);
  return modelNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLPolyData::DeviceToMRMLInternal(igtlioDevice* device,
                                           vtkMRMLNode* node,
                                           vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioPolyDataDevice* polyDevice = reinterpret_cast<igtlioPolyDataDevice*>(device);
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode)
  {
    return;
  }
  modelNode->SetAndObservePolyData(polyDevice->GetContent().polydata);
  modelNode->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLPolyData::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioPolyDataDevice* polyDevice = static_cast<igtlioPolyDataDevice*>(device);
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  igtlioPolyDataConverter::ContentData content = { modelNode->GetPolyData(), modelNode->GetName() };
  polyDevice->SetContent(content);
  return vtkMRMLModelNode::MeshModifiedEvent;
}
