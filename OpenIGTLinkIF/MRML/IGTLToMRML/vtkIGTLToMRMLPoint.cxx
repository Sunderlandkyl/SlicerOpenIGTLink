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

#include "vtkIGTLToMRMLPoint.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioPointDevice.h>

// MRML includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkVector.h>

vtkStandardNewMacro(vtkIGTLToMRMLPoint);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLPoint::GetMRMLNodeTagNames()
{
  return { "MarkupsFiducial" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLPoint::CreateNode(vtkMRMLScene* scene,
                                              igtlioDevice* device,
                                              vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLMarkupsFiducialNode> markupsNode =
    vtkMRMLMarkupsFiducialNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLMarkupsFiducialNode"));
  if (markupsNode)
  {
    connector->RegisterIncomingMRMLNode(markupsNode, device);
    return markupsNode;
  }

  markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    scene->AddNewNodeByClass("vtkMRMLMarkupsFiducialNode"));
  markupsNode->SetName(deviceName.c_str());
  markupsNode->SetDescription("Received by OpenIGTLink");
  markupsNode->CreateDefaultDisplayNodes();
  connector->RegisterIncomingMRMLNode(markupsNode, device);
  return markupsNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLPoint::DeviceToMRML(igtlioDevice* device,
                                        vtkMRMLNode* node,
                                        vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioPointDevice* pointDevice = reinterpret_cast<igtlioPointDevice*>(device);
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
  if (!markupsNode)
  {
    return;
  }

  double selectedColor[3] = { 1.0, 1.0, 1.0 };
  double unselectedColor[3] = { 1.0, 1.0, 1.0 };
  bool selectedColorDefined = false;
  bool unselectedColorDefined = false;

  MRMLNodeModifyBlocker blocker(markupsNode);

  const igtlioPointConverter::PointList& points = pointDevice->GetContent().PointElements;

  // Remove unneeded existing points
  int numberOfPointsToRemove = markupsNode->GetNumberOfControlPoints() - static_cast<int>(points.size());
  for (int i = 0; i < numberOfPointsToRemove; i++)
  {
    markupsNode->RemoveNthControlPoint(markupsNode->GetNumberOfControlPoints() - 1);
  }

  // Add/update control points
  for (int controlPointIndex = 0; controlPointIndex < static_cast<int>(points.size()); controlPointIndex++)
  {
    const igtlioPointConverter::PointElement& point = points[controlPointIndex];
    bool selected = (point.GroupName != "Unselected");

    if (selected && !selectedColorDefined)
    {
      selectedColor[0] = point.RGBA[0] / 255.0;
      selectedColor[1] = point.RGBA[1] / 255.0;
      selectedColor[2] = point.RGBA[2] / 255.0;
      selectedColorDefined = true;
    }
    else if (!selected && !unselectedColorDefined)
    {
      unselectedColor[0] = point.RGBA[0] / 255.0;
      unselectedColor[1] = point.RGBA[1] / 255.0;
      unselectedColor[2] = point.RGBA[2] / 255.0;
      unselectedColorDefined = true;
    }

    if (markupsNode->GetNumberOfControlPoints() <= controlPointIndex)
    {
      markupsNode->AddControlPoint(
        vtkVector3d(point.Position[0], point.Position[1], point.Position[2]), point.Name);
    }
    else
    {
      markupsNode->SetNthControlPointLabel(controlPointIndex, point.Name);
      markupsNode->SetNthControlPointPosition(
        controlPointIndex, point.Position[0], point.Position[1], point.Position[2]);
    }
    markupsNode->SetNthControlPointSelected(controlPointIndex, selected);
    markupsNode->SetNthControlPointVisibility(controlPointIndex, point.RGBA[3] > 0);
  }

  vtkMRMLMarkupsDisplayNode* displayNode =
    vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetDisplayNode());
  if (displayNode)
  {
    MRMLNodeModifyBlocker displayBlocker(displayNode);
    if (selectedColorDefined)
    {
      displayNode->SetSelectedColor(selectedColor);
    }
    if (unselectedColorDefined)
    {
      displayNode->SetColor(unselectedColor);
    }
  }
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLPoint::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioPointDevice* pointDevice = static_cast<igtlioPointDevice*>(device);
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
  vtkMRMLMarkupsDisplayNode* displayNode =
    vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetDisplayNode());

  igtlioPointConverter::ContentData content;
  for (int controlPointIndex = 0; controlPointIndex < markupsNode->GetNumberOfControlPoints(); controlPointIndex++)
  {
    igtlioPointConverter::PointElement point;
    point.Name = markupsNode->GetNthControlPointLabel(controlPointIndex);

    bool pointSelected = markupsNode->GetNthControlPointSelected(controlPointIndex);
    point.GroupName = (pointSelected ? "Selected" : "Unselected");
    if (displayNode)
    {
      double* color = (pointSelected ? displayNode->GetSelectedColor() : displayNode->GetColor());
      point.RGBA[0] = static_cast<int>(color[0] * 255);
      point.RGBA[1] = static_cast<int>(color[1] * 255);
      point.RGBA[2] = static_cast<int>(color[2] * 255);
    }
    else
    {
      if (pointSelected)
      {
        point.RGBA[0] = 255; point.RGBA[1] = 0;   point.RGBA[2] = 0;
      }
      else
      {
        point.RGBA[0] = 230; point.RGBA[1] = 230; point.RGBA[2] = 77;
      }
    }
    point.RGBA[3] = markupsNode->GetNthControlPointVisibility(controlPointIndex) ? 255 : 0;

    double position[3] = { 0.0 };
    markupsNode->GetNthControlPointPosition(controlPointIndex, position);
    point.Position[0] = position[0];
    point.Position[1] = position[1];
    point.Position[2] = position[2];
    point.Radius = 0.0;
    content.PointElements.push_back(point);
  }
  pointDevice->SetContent(content);
  return vtkMRMLMarkupsNode::PointModifiedEvent;
}
