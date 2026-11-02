/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University.

==============================================================================*/

// Unit test for the metadata handling in vtkIGTLToMRMLBase and its subclasses.
// No network connection is needed: converters are exercised directly against
// in-memory igtlio devices and MRML nodes.

// OpenIGTLinkIO includes
#include <igtlioImageDevice.h>
#include <igtlioTransformDevice.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLIOConverterFactory.h"
#include "vtkIGTLToMRMLImage.h"
#include "vtkIGTLToMRMLTransform.h"

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include "vtkMRMLCoreTestingMacros.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

namespace
{

//----------------------------------------------------------------------------
int TestStatusMetaDataKeys()
{
  vtkNew<vtkIGTLToMRMLImage> imageConverter;
  vtkNew<vtkIGTLToMRMLTransform> transformConverter;

  CHECK_STRING(imageConverter->GetStatusMetaDataKey(), "Status");
  CHECK_STRING(transformConverter->GetStatusMetaDataKey(), "TransformStatus");
  CHECK_STRING(vtkIGTLToMRMLBase::MRMLNodeNameKey, "MRMLNodeName");
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestPrepareOutgoingTransformDevice()
{
  vtkNew<vtkIGTLToMRMLTransform> converter;
  vtkNew<vtkMRMLLinearTransformNode> transformNode;
  transformNode->SetName("TestTransform");
  vtkSmartPointer<igtlioTransformDevice> device = vtkSmartPointer<igtlioTransformDevice>::New();
  device->SetDeviceName("TestTransform");

  std::string value;

  // No status attribute: the node is assumed valid and status defaults to "OK".
  converter->PrepareOutgoingDevice(transformNode, device);
  CHECK_BOOL(device->GetMetaDataElement(vtkIGTLToMRMLBase::MRMLNodeNameKey, value), true);
  CHECK_STD_STRING(value, "LinearTransform");
  CHECK_BOOL(device->GetMetaDataElement("TransformStatus", value), true);
  CHECK_STD_STRING(value, "OK");
  // The generic "Status" key must not be written for transforms.
  CHECK_BOOL(device->GetMetaDataElement("Status", value), false);

  // Status attribute present: it is forwarded verbatim.
  device->ClearMetaData();
  transformNode->SetAttribute("TransformStatus", "MISSING");
  converter->PrepareOutgoingDevice(transformNode, device);
  CHECK_BOOL(device->GetMetaDataElement("TransformStatus", value), true);
  CHECK_STD_STRING(value, "MISSING");

  // A "Status" attribute on a transform node is ignored; only TransformStatus matters.
  device->ClearMetaData();
  transformNode->RemoveAttribute("TransformStatus");
  transformNode->SetAttribute("Status", "ERROR");
  converter->PrepareOutgoingDevice(transformNode, device);
  CHECK_BOOL(device->GetMetaDataElement("TransformStatus", value), true);
  CHECK_STD_STRING(value, "OK");
  CHECK_BOOL(device->GetMetaDataElement("Status", value), false);

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestPrepareOutgoingImageDevice()
{
  vtkNew<vtkIGTLToMRMLImage> converter;
  vtkNew<vtkMRMLScalarVolumeNode> volumeNode;
  volumeNode->SetName("TestImage");
  vtkSmartPointer<igtlioImageDevice> device = vtkSmartPointer<igtlioImageDevice>::New();
  device->SetDeviceName("TestImage");

  std::string value;

  converter->PrepareOutgoingDevice(volumeNode, device);
  CHECK_BOOL(device->GetMetaDataElement(vtkIGTLToMRMLBase::MRMLNodeNameKey, value), true);
  CHECK_STD_STRING(value, "Volume");
  CHECK_BOOL(device->GetMetaDataElement("Status", value), true);
  CHECK_STD_STRING(value, "OK");
  CHECK_BOOL(device->GetMetaDataElement("TransformStatus", value), false);

  device->ClearMetaData();
  volumeNode->SetAttribute("Status", "ERROR");
  converter->PrepareOutgoingDevice(volumeNode, device);
  CHECK_BOOL(device->GetMetaDataElement("Status", value), true);
  CHECK_STD_STRING(value, "ERROR");

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestDeviceToMRMLCopiesMetaData()
{
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkIGTLToMRMLTransform> converter;

  vtkSmartPointer<igtlioTransformDevice> device = vtkSmartPointer<igtlioTransformDevice>::New();
  device->SetDeviceName("TestTransform");
  vtkNew<vtkMatrix4x4> matrix;
  matrix->SetElement(0, 3, 12.5);
  matrix->SetElement(1, 3, -3.0);
  igtlioTransformConverter::ContentData content = { matrix.GetPointer(), "TestTransform", "", "" };
  device->SetContent(content);
  device->SetMetaDataElement(vtkIGTLToMRMLBase::MRMLNodeNameKey, IANA_TYPE_US_ASCII, "LinearTransform");
  device->SetMetaDataElement("TransformStatus", IANA_TYPE_US_ASCII, "MISSING");
  device->SetMetaDataElement("CustomKey", IANA_TYPE_US_ASCII, "CustomValue");

  vtkNew<vtkMRMLLinearTransformNode> transformNode;
  scene->AddNode(transformNode);

  converter->DeviceToMRML(device, transformNode, nullptr);

  // Content was transferred by the subclass.
  vtkNew<vtkMatrix4x4> received;
  transformNode->GetMatrixTransformToParent(received);
  CHECK_DOUBLE(received->GetElement(0, 3), 12.5);
  CHECK_DOUBLE(received->GetElement(1, 3), -3.0);

  // Every metadata element was copied as an "OpenIGTLink." prefixed attribute by the base class.
  CHECK_STRING(transformNode->GetAttribute("OpenIGTLink.MRMLNodeName"), "LinearTransform");
  CHECK_STRING(transformNode->GetAttribute("OpenIGTLink.TransformStatus"), "MISSING");
  CHECK_STRING(transformNode->GetAttribute("OpenIGTLink.CustomKey"), "CustomValue");

  // The transform converter additionally mirrors status metadata without the prefix,
  // so that it can be fed straight back into PrepareOutgoingDevice.
  CHECK_STRING(transformNode->GetAttribute("TransformStatus"), "MISSING");

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestConverterFactoryRegistration()
{
  vtkIGTLIOConverterFactory* factory = vtkIGTLIOConverterFactory::GetInstance();
  CHECK_NOT_NULL(factory);

  vtkSmartPointer<vtkIGTLToMRMLTransform> transformConverter = vtkSmartPointer<vtkIGTLToMRMLTransform>::New();
  factory->RegisterConverter(transformConverter);
  CHECK_POINTER(factory->GetConverter("TRANSFORM"), transformConverter.GetPointer());

  std::vector<std::string> tags = factory->GetNodeTagsFromDeviceType("TRANSFORM");
  CHECK_INT(static_cast<int>(tags.size()), 1);
  CHECK_STD_STRING(tags[0], "LinearTransform");

  std::vector<std::string> deviceTypes = factory->GetDeviceTypesFromNodeTag("LinearTransform");
  CHECK_INT(static_cast<int>(deviceTypes.size()), 1);
  CHECK_STD_STRING(deviceTypes[0], "TRANSFORM");

  factory->UnregisterConverter("TRANSFORM");
  CHECK_NULL(factory->GetConverter("TRANSFORM"));
  CHECK_INT(static_cast<int>(factory->GetNodeTagsFromDeviceType("TRANSFORM").size()), 0);

  // Unregistering an unknown type is a no-op.
  factory->UnregisterConverter("DOES_NOT_EXIST");

  return EXIT_SUCCESS;
}

} // namespace

//----------------------------------------------------------------------------
int vtkIGTLToMRMLConverterMetaDataTest(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  CHECK_EXIT_SUCCESS(TestStatusMetaDataKeys());
  CHECK_EXIT_SUCCESS(TestPrepareOutgoingTransformDevice());
  CHECK_EXIT_SUCCESS(TestPrepareOutgoingImageDevice());
  CHECK_EXIT_SUCCESS(TestDeviceToMRMLCopiesMetaData());
  CHECK_EXIT_SUCCESS(TestConverterFactoryRegistration());
  std::cout << "Test passed." << std::endl;
  return EXIT_SUCCESS;
}
