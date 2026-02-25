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

#include "vtkIGTLToMRMLString.h"
#include "vtkMRMLIGTLConnectorNode.h"

// OpenIGTLinkIO includes
#include <igtlioStringDevice.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLTextNode.h>

// VTK includes
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkIGTLToMRMLString);

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLToMRMLString::GetMRMLNodeTagNames()
{
  return { "Text" };
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLString::CreateNode(vtkMRMLScene* scene,
                                               igtlioDevice* device,
                                               vtkMRMLIGTLConnectorNode* connector)
{
  std::string deviceName = device->GetDeviceName().empty() ? "OpenIGTLink" : device->GetDeviceName();

  vtkSmartPointer<vtkMRMLTextNode> textNode =
    vtkMRMLTextNode::SafeDownCast(scene->GetFirstNode(deviceName.c_str(), "vtkMRMLTextNode"));
  if (textNode)
  {
    connector->RegisterIncomingMRMLNode(textNode, device);
    return textNode;
  }

  igtlioStringDevice* stringDevice = reinterpret_cast<igtlioStringDevice*>(device);
  textNode = vtkSmartPointer<vtkMRMLTextNode>::Take(
    vtkMRMLTextNode::SafeDownCast(scene->CreateNodeByClass("vtkMRMLTextNode")));
  textNode->SetEncoding(stringDevice->GetContent().encoding);
  textNode->SetText(stringDevice->GetContent().string_msg.c_str());
  textNode->SetName(deviceName.c_str());
  textNode->SetDescription("Received by OpenIGTLink");
  scene->AddNode(textNode);
  connector->RegisterIncomingMRMLNode(textNode, device);
  return textNode;
}

//----------------------------------------------------------------------------
void vtkIGTLToMRMLString::DeviceToMRML(igtlioDevice* device,
                                         vtkMRMLNode* node,
                                         vtkMRMLIGTLConnectorNode* vtkNotUsed(connector))
{
  igtlioStringDevice* stringDevice = reinterpret_cast<igtlioStringDevice*>(device);
  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(node);
  if (!textNode)
  {
    return;
  }
  textNode->SetEncoding(stringDevice->GetContent().encoding);
  textNode->SetText(stringDevice->GetContent().string_msg.c_str());
  textNode->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkIGTLToMRMLString::MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device)
{
  igtlioStringDevice* stringDevice = static_cast<igtlioStringDevice*>(device);
  vtkMRMLTextNode* textNode = vtkMRMLTextNode::SafeDownCast(node);
  std::string text = textNode->GetText();
  igtlioStringConverter::ContentData content = {
    static_cast<unsigned int>(textNode->GetEncoding()), text
  };
  stringDevice->SetContent(content);
  return vtkMRMLTextNode::TextModifiedEvent;
}
