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

// Tests the query waiting queue of vtkMRMLIGTLConnectorNode:
//  - deleted query nodes are purged from the queue,
//  - queries past their timeout are marked expired and removed,
//  - queries that are no longer waiting are removed,
//  - observers that re-enter the connector from a query node Modified() event
//    (fired while the connector updates the node) do not deadlock.
// A server connector with no clients is enough; no network traffic is required.

// OpenIGTLinkIF MRML includes
#include "vtkIGTLIOConverterFactory.h"
#include "vtkIGTLToMRMLImage.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLQueryNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include "vtkMRMLCoreTestingMacros.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtksys/SystemTools.hxx>

namespace
{

const int TEST_PORT = 18947;

//----------------------------------------------------------------------------
// SetConnectorNodeID("") removes the node reference, so the getter may return
// either nullptr or an empty string.
bool IsEmptyString(const char* str)
{
  return str == nullptr || str[0] == '\0';
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMRMLIGTLQueryNode> CreateQueryNode(vtkMRMLScene* scene, const char* name)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = vtkSmartPointer<vtkMRMLIGTLQueryNode>::New();
  queryNode->SetName(name);
  // IMAGE devices support the GET prefix, so a GET_IMAGE query is well formed.
  queryNode->SetIGTLName("IMAGE");
  queryNode->SetIGTLDeviceName(name);
  queryNode->SetQueryType(vtkMRMLIGTLQueryNode::TYPE_GET);
  scene->AddNode(queryNode);
  return queryNode;
}

//----------------------------------------------------------------------------
int TestDeletedQueryNodeIsPurged(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  {
    vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "DeletedQuery");
    CHECK_INT(connector->PushQuery(queryNode), 0);
    CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);
    CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_WAITING);
    CHECK_STRING(queryNode->GetConnectorNodeID(), connector->GetID());
    scene->RemoveNode(queryNode);
    // queryNode goes out of scope here: the weak pointer in the queue becomes nullptr.
  }
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);
  CHECK_NULL(connector->QueryWaitingQueue.front().GetPointer());

  connector->PeriodicProcess();
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestExpiredQueryIsRemoved(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "ExpiringQuery");
  queryNode->SetTimeOut(0.05); // seconds
  CHECK_INT(connector->PushQuery(queryNode), 0);
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);

  // Not yet expired
  connector->PeriodicProcess();
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);
  CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_WAITING);

  vtksys::SystemTools::Delay(150);
  connector->PeriodicProcess();
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);
  CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_EXPIRED);

  scene->RemoveNode(queryNode);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestQueryWithoutTimeoutIsKept(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "PatientQuery");
  queryNode->SetTimeOut(0.0);
  CHECK_INT(connector->PushQuery(queryNode), 0);

  vtksys::SystemTools::Delay(50);
  connector->PeriodicProcess();
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);
  CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_WAITING);

  connector->CancelQuery(queryNode);
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);
  CHECK_BOOL(IsEmptyString(queryNode->GetConnectorNodeID()), true);

  scene->RemoveNode(queryNode);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestFinishedQueryIsRemoved(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "FinishedQuery");
  CHECK_INT(connector->PushQuery(queryNode), 0);
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);

  // Something else (e.g. a response handler) marks the query as done.
  queryNode->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_SUCCESS);
  connector->PeriodicProcess();
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);
  // Status is left untouched for non-expired queries.
  CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_SUCCESS);

  scene->RemoveNode(queryNode);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
// Observer that re-enters the connector from the query node's Modified() event.
struct ReentrantObserverData
{
  vtkMRMLIGTLConnectorNode* Connector = nullptr;
  int TriggerStatus = vtkMRMLIGTLQueryNode::STATUS_NOT_DEFINED;
  int CallCount = 0;
};

void OnQueryNodeModifiedCancel(vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientData, void* vtkNotUsed(callData))
{
  ReentrantObserverData* data = static_cast<ReentrantObserverData*>(clientData);
  vtkMRMLIGTLQueryNode* queryNode = vtkMRMLIGTLQueryNode::SafeDownCast(caller);
  if (!queryNode || queryNode->GetQueryStatus() != data->TriggerStatus || data->CallCount > 0)
  {
    return;
  }
  data->CallCount++;
  // Re-enter the connector while it is still inside PushQuery / RemoveExpiredQueries.
  // With the query queue mutex held across node setters this would deadlock.
  data->Connector->CancelQuery(queryNode);
}

//----------------------------------------------------------------------------
int TestReentrantObserverOnPushQuery(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "ReentrantPushQuery");

  ReentrantObserverData data;
  data.Connector = connector;
  data.TriggerStatus = vtkMRMLIGTLQueryNode::STATUS_WAITING;
  vtkNew<vtkCallbackCommand> callback;
  callback->SetCallback(OnQueryNodeModifiedCancel);
  callback->SetClientData(&data);
  unsigned long tag = queryNode->AddObserver(vtkCommand::ModifiedEvent, callback);

  CHECK_INT(connector->PushQuery(queryNode), 0);
  CHECK_INT(data.CallCount, 1);
  // The observer cancelled the query from inside PushQuery; it must not be in the queue.
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);

  queryNode->RemoveObserver(tag);
  scene->RemoveNode(queryNode);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestReentrantObserverOnExpiry(vtkMRMLIGTLConnectorNode* connector, vtkMRMLScene* scene)
{
  vtkSmartPointer<vtkMRMLIGTLQueryNode> queryNode = CreateQueryNode(scene, "ReentrantExpiryQuery");
  queryNode->SetTimeOut(0.05);

  ReentrantObserverData data;
  data.Connector = connector;
  data.TriggerStatus = vtkMRMLIGTLQueryNode::STATUS_EXPIRED;
  vtkNew<vtkCallbackCommand> callback;
  callback->SetCallback(OnQueryNodeModifiedCancel);
  callback->SetClientData(&data);
  unsigned long tag = queryNode->AddObserver(vtkCommand::ModifiedEvent, callback);

  CHECK_INT(connector->PushQuery(queryNode), 0);
  CHECK_INT(data.CallCount, 0);
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 1);

  vtksys::SystemTools::Delay(150);
  connector->PeriodicProcess();
  CHECK_INT(data.CallCount, 1);
  CHECK_INT(queryNode->GetQueryStatus(), vtkMRMLIGTLQueryNode::STATUS_EXPIRED);
  CHECK_INT(static_cast<int>(connector->QueryWaitingQueue.size()), 0);
  CHECK_BOOL(IsEmptyString(queryNode->GetConnectorNodeID()), true);

  queryNode->RemoveObserver(tag);
  scene->RemoveNode(queryNode);
  return EXIT_SUCCESS;
}

} // namespace

//----------------------------------------------------------------------------
int vtkMRMLConnectorQueryQueueTest(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Converters are normally registered by the module logic; the test links only the MRML library.
  vtkIGTLIOConverterFactory::GetInstance()->RegisterConverter(vtkSmartPointer<vtkIGTLToMRMLImage>::New());

  vtkNew<vtkMRMLScene> scene;
  vtkSmartPointer<vtkMRMLIGTLConnectorNode> connector = vtkSmartPointer<vtkMRMLIGTLConnectorNode>::New();
  scene->AddNode(connector);
  connector->SetTypeServer(TEST_PORT);
  connector->Start();
  CHECK_BOOL(connector->GetState() != vtkMRMLIGTLConnectorNode::StateOff, true);

  int result = EXIT_SUCCESS;
  if (TestDeletedQueryNodeIsPurged(connector, scene) != EXIT_SUCCESS
    || TestExpiredQueryIsRemoved(connector, scene) != EXIT_SUCCESS
    || TestQueryWithoutTimeoutIsKept(connector, scene) != EXIT_SUCCESS
    || TestFinishedQueryIsRemoved(connector, scene) != EXIT_SUCCESS
    || TestReentrantObserverOnPushQuery(connector, scene) != EXIT_SUCCESS
    || TestReentrantObserverOnExpiry(connector, scene) != EXIT_SUCCESS)
  {
    result = EXIT_FAILURE;
  }

  connector->Stop();
  scene->Clear(1);

  if (result == EXIT_SUCCESS)
  {
    std::cout << "Test passed." << std::endl;
  }
  return result;
}
