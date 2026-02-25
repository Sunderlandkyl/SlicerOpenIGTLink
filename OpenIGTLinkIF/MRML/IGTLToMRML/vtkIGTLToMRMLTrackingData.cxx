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

#include "vtkIGTLToMRMLTrackingData.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"

// OpenIGTLinkIO includes
#include <igtlioTrackingDataDevice.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLTrackingData);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLTrackingData::GetMRMLNodeTagNames()
{
  return { "IGTLTrackingDataSplitter" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLTrackingData::CreateNode(vtkMRMLScene* scene,
                                                     igtlioDevice* device,
                                                     vtkMRMLIGTLConnectorNode* connector)
{
  igtlioTrackingDataDevice* tdata = dynamic_cast<igtlioTrackingDataDevice*>(device);
  if (!tdata)
  {
    vtkErrorWithObjectMacro(device, "TDATA message type but not a TDATA device. Cannot process.");
    return nullptr;
  }

  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLIGTLTrackingDataBundleNode> tdataNode =
    vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(
      scene->GetFirstNode(deviceName.c_str(), "vtkMRMLIGTLTrackingDataBundleNode"));
  if (tdataNode)
  {
    connector->RegisterIncomingMRMLNode(tdataNode, device);
    return tdataNode;
  }

  auto contentCopy = tdata->GetContent();
  tdataNode = vtkSmartPointer<vtkMRMLIGTLTrackingDataBundleNode>::Take(
    vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(
      scene->CreateNodeByClass("vtkMRMLIGTLTrackingDataBundleNode")));
  tdataNode->SetName(deviceName.c_str());
  tdataNode->SetDescription("Received by OpenIGTLink");
  for (auto iter = contentCopy.trackingDataElements.cbegin();
       iter != contentCopy.trackingDataElements.cend(); ++iter)
  {
    tdataNode->UpdateTransformNode(iter->second.deviceName.c_str(),
                                    iter->second.transform,
                                    iter->second.type);
  }
  scene->AddNode(tdataNode);
  connector->RegisterIncomingMRMLNode(tdataNode, device);
  return tdataNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLTrackingData::DeviceToMRMLInternal(igtlioDevice* device,
                                               vtkMRMLNode* node,
                                               vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioTrackingDataDevice* tdataDevice = reinterpret_cast<igtlioTrackingDataDevice*>(device);
  vtkMRMLIGTLTrackingDataBundleNode* tBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(node);
  if (!tBundleNode)
  {
    return;
  }

  int nElements = tBundleNode->GetNumberOfTransformNodes();
  igtlioTrackingDataConverter::ContentData content = tdataDevice->GetContent();

  // Process in reverse order so that only the first occurrence of each transform name is used
  // if duplicates are present.
  for (auto iter = content.trackingDataElements.rbegin();
       iter != content.trackingDataElements.rend(); ++iter)
  {
    bool found = false;
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int i = 0; i < nElements; i++)
    {
      vtkMRMLLinearTransformNode* transformNode = tBundleNode->GetTransformNode(i);
      if (iter->second.deviceName.compare(transformNode->GetName()) == 0)
      {
        found = true;
        mat->DeepCopy(iter->second.transform);
        transformNode->SetMatrixTransformToParent(mat.GetPointer());
        transformNode->Modified();
        break;
      }
    }
    if (!found)
    {
      tBundleNode->UpdateTransformNode(iter->second.deviceName.c_str(), mat, iter->second.type);
      nElements = tBundleNode->GetNumberOfTransformNodes();
    }
  }
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLTrackingData::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioTrackingDataDevice* tdataDevice = static_cast<igtlioTrackingDataDevice*>(device);
  vtkMRMLIGTLTrackingDataBundleNode* tBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(node);
  if (!tBundleNode)
  {
    return 0;
  }

  igtlioTrackingDataConverter::ContentData content = tdataDevice->GetContent();
  int nElements = tBundleNode->GetNumberOfTransformNodes();
  for (int i = 0; i < nElements; i++)
  {
    vtkMRMLLinearTransformNode* transformNode = tBundleNode->GetTransformNode(i);
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    transformNode->GetMatrixTransformToParent(mat);
    bool found = false;
    for (auto iter = content.trackingDataElements.begin();
         iter != content.trackingDataElements.end(); ++iter)
    {
      if (iter->second.deviceName.compare(transformNode->GetName()) == 0)
      {
        found = true;
        iter->second.transform = mat;
        break;
      }
    }
    if (!found)
    {
      int idx = static_cast<int>(content.trackingDataElements.size());
      content.trackingDataElements[idx] = igtlioTrackingDataConverter::ContentEntry(
        mat, transformNode->GetName(), transformNode->GetName());
    }
  }
  tdataDevice->SetContent(content);
  return vtkCommand::ModifiedEvent;
}
