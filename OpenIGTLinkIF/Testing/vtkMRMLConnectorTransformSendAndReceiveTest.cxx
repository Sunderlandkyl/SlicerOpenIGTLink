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

// End-to-end test: a server connector pushes a linear transform node and a
// client connector in a separate scene receives it. Verifies that the matrix,
// the MRMLNodeName metadata and the TransformStatus metadata survive the round
// trip through the converter factory (PrepareOutgoingDevice on the sending side,
// DeviceToMRML + metadata copy on the receiving side).

// OpenIGTLink includes
#include "igtlOSUtil.h"

// OpenIGTLinkIO includes
#include <igtlioDevice.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLIOConverterFactory.h"
#include "vtkIGTLToMRMLTransform.h"
#include "vtkMRMLIGTLConnectorNode.h"

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScene.h>
#include "vtkMRMLCoreTestingMacros.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
#include <vtksys/SystemTools.hxx>

namespace
{

const int TEST_PORT = 18948;
const double TIMEOUT_SEC = 5.0;

//----------------------------------------------------------------------------
struct TestFixture
{
  vtkNew<vtkMRMLScene> ServerScene;
  vtkNew<vtkMRMLScene> ClientScene;
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> Server;
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> Client;

  ~TestFixture()
  {
    if (this->Client)
    {
      this->Client->Stop();
    }
    if (this->Server)
    {
      this->Server->Stop();
    }
  }

  void Process()
  {
    this->Server->PeriodicProcess();
    this->Client->PeriodicProcess();
    vtksys::SystemTools::Delay(5);
  }
};

//----------------------------------------------------------------------------
int Connect(TestFixture& f)
{
  f.Server = vtkSmartPointer<vtkMRMLIGTLConnectorNode>::New();
  f.ServerScene->AddNode(f.Server);
  f.Server->SetTypeServer(TEST_PORT);
  f.Server->Start();
  igtl::Sleep(20);

  f.Client = vtkSmartPointer<vtkMRMLIGTLConnectorNode>::New();
  f.ClientScene->AddNode(f.Client);
  f.Client->SetTypeClient("localhost", TEST_PORT);
  f.Client->Start();

  double startTime = vtkTimerLog::GetUniversalTime();
  while (vtkTimerLog::GetUniversalTime() - startTime < TIMEOUT_SEC)
  {
    f.Process();
    if (f.Client->GetState() == vtkMRMLIGTLConnectorNode::StateConnected)
    {
      std::cout << "Client connected to server" << std::endl;
      return EXIT_SUCCESS;
    }
    if (f.Client->GetState() == vtkMRMLIGTLConnectorNode::StateOff)
    {
      break;
    }
  }
  std::cerr << "Client failed to connect to server" << std::endl;
  return EXIT_FAILURE;
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* WaitForReceivedTransform(TestFixture& f, const char* name)
{
  double startTime = vtkTimerLog::GetUniversalTime();
  while (vtkTimerLog::GetUniversalTime() - startTime < TIMEOUT_SEC)
  {
    f.Process();
    vtkMRMLLinearTransformNode* node = vtkMRMLLinearTransformNode::SafeDownCast(
      f.ClientScene->GetFirstNode(name, "vtkMRMLLinearTransformNode"));
    // The transform is created on the first message and filled in by DeviceToMRML;
    // wait until the metadata copy has happened so the check below is not racy.
    if (node && node->GetAttribute("OpenIGTLink.MRMLNodeName"))
    {
      return node;
    }
  }
  return nullptr;
}

} // namespace

//----------------------------------------------------------------------------
int vtkMRMLConnectorTransformSendAndReceiveTest(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Converters are normally registered by the module logic; the test links only the MRML library.
  vtkIGTLIOConverterFactory::GetInstance()->RegisterConverter(vtkSmartPointer<vtkIGTLToMRMLTransform>::New());

  TestFixture f;
  CHECK_EXIT_SUCCESS(Connect(f));

  // Outgoing node on the server side
  const char* transformName = "TestTransform";
  vtkSmartPointer<vtkMRMLLinearTransformNode> sentNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  sentNode->SetName(transformName);
  vtkNew<vtkMatrix4x4> sentMatrix;
  sentMatrix->SetElement(0, 3, 10.0);
  sentMatrix->SetElement(1, 3, 20.0);
  sentMatrix->SetElement(2, 3, 30.0);
  sentNode->SetMatrixTransformToParent(sentMatrix);
  sentNode->SetAttribute("TransformStatus", "MISSING");
  f.ServerScene->AddNode(sentNode);

  igtlioDevice* device = reinterpret_cast<igtlioDevice*>(f.Server->CreateDeviceForOutgoingMRMLNode(sentNode));
  CHECK_NOT_NULL(device);
  CHECK_STD_STRING(device->GetDeviceType(), "TRANSFORM");
  CHECK_STD_STRING(device->GetDeviceName(), transformName);

  CHECK_INT(f.Server->PushNode(sentNode), 0);

  // Metadata was attached to the outgoing device by the converter
  std::string value;
  CHECK_BOOL(device->GetMetaDataElement(vtkIGTLToMRMLBase::MRMLNodeNameKey, value), true);
  CHECK_STD_STRING(value, "LinearTransform");
  CHECK_BOOL(device->GetMetaDataElement("TransformStatus", value), true);
  CHECK_STD_STRING(value, "MISSING");

  // Incoming node on the client side
  vtkMRMLLinearTransformNode* receivedNode = WaitForReceivedTransform(f, transformName);
  CHECK_NOT_NULL(receivedNode);
  CHECK_STRING(receivedNode->GetName(), transformName);

  vtkNew<vtkMatrix4x4> receivedMatrix;
  receivedNode->GetMatrixTransformToParent(receivedMatrix);
  CHECK_DOUBLE(receivedMatrix->GetElement(0, 3), 10.0);
  CHECK_DOUBLE(receivedMatrix->GetElement(1, 3), 20.0);
  CHECK_DOUBLE(receivedMatrix->GetElement(2, 3), 30.0);

  // Metadata copied by vtkIGTLToMRMLBase::DeviceToMRML
  CHECK_STRING(receivedNode->GetAttribute("OpenIGTLink.MRMLNodeName"), "LinearTransform");
  CHECK_STRING(receivedNode->GetAttribute("OpenIGTLink.TransformStatus"), "MISSING");
  // Status mirrored without prefix by the transform converter
  CHECK_STRING(receivedNode->GetAttribute("TransformStatus"), "MISSING");

  // Push again with a changed status and make sure the update propagates
  sentNode->SetAttribute("TransformStatus", "OK");
  sentMatrix->SetElement(0, 3, 11.0);
  sentNode->SetMatrixTransformToParent(sentMatrix);
  CHECK_INT(f.Server->PushNode(sentNode), 0);

  double startTime = vtkTimerLog::GetUniversalTime();
  bool updated = false;
  while (vtkTimerLog::GetUniversalTime() - startTime < TIMEOUT_SEC)
  {
    f.Process();
    receivedNode->GetMatrixTransformToParent(receivedMatrix);
    const char* status = receivedNode->GetAttribute("TransformStatus");
    if (receivedMatrix->GetElement(0, 3) == 11.0 && status && strcmp(status, "OK") == 0)
    {
      updated = true;
      break;
    }
  }
  CHECK_BOOL(updated, true);
  // The client scene must still contain a single transform: the update went to the existing node.
  CHECK_INT(f.ClientScene->GetNumberOfNodesByClass("vtkMRMLLinearTransformNode"), 1);

  std::cout << "Test passed." << std::endl;
  return EXIT_SUCCESS;
}
