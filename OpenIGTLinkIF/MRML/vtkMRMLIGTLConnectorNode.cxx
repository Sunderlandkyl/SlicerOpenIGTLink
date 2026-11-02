/*=auto=========================================================================

Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

See Doc/copyright/copyright.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLConnectorNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// OpenIGTLink includes
#include "igtlOSUtil.h"

// OpenIGTLinkIO include
#include <igtlioCommand.h>
#include <igtlioConnector.h>
#include <igtlioDeviceFactory.h>
#include <igtlioImageDevice.h>
#include <igtlioImageMetaDevice.h>
#include <igtlioLabelMetaDevice.h>
#include <igtlioPolyDataDevice.h>
#include <igtlioPointDevice.h>
#include <igtlioStatusDevice.h>
#include <igtlioStringDevice.h>
#include <igtlioTransformDevice.h>
#include <igtlioTrackingDataDevice.h>
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
#include <igtlioVideoDevice.h>
#endif

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLStatusNode.h"
#include "vtkMRMLImageMetaListNode.h"
#include "vtkMRMLLabelMetaListNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLTextNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkSlicerOpenIGTLinkCommand.h"
#include "vtkIGTLIOConverterFactory.h"

// MRML includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLStreamingVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>
#include <vtkWeakPointer.h>

// vtksys includes
#include <vtksys/SystemTools.hxx>

// std includes
#include <mutex>

#define OriginalNodeNameKey "OriginalNodeName"

// The macro SendMessage in winuser.h clashes with the method name in igtlioConnector.
// A simple workaround is to undefine this macro to avoid name conflict (https://stackoverflow.com/a/69041270)
#pragma push_macro("SendMessage")
#undef SendMessage

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorNode);

//---------------------------------------------------------------------------
class vtkMRMLIGTLConnectorNode::vtkInternal
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLIGTLConnectorNode* external);
  virtual ~vtkInternal();

  ///  Send the given command from the given device.
  /// - If using BLOCKING, the call blocks until a response appears or timeout. Return response.
  /// - If using ASYNCHRONOUS, wait for the CommandResponseReceivedEvent event. Return device.
  ///
  igtlioCommandPointer SendCommand(igtlioCommandPointer command);

  /// Send a command response from the given device. Asynchronous.
  /// Precondition: The given device has received a query that is not yet responded to.
  /// Return device.
  int SendCommandResponse(igtlioCommandPointer command);

  void ProcessNewDeviceEvent(vtkObject* caller, unsigned long event, void* callData);

  void ProcessOutgoingDeviceModifiedEvent(vtkObject* caller, unsigned long event, igtlioDevice* modifiedDevice);

  void DeviceAboutToReceiveEvent(igtlioDevice* modifiedDevice);

  /// Called when command responses are received.
  /// Handles igtlioCommandPointer related details on command response.
  /// If there is a corresponding igtlioCommandPointer, an event is invoked on it.
  void ReceiveCommandResponse(igtlioCommandPointer commandDevice);

  /// Find the first relevant query node for a given device in the list of pending queries
  vtkMRMLIGTLQueryNode* GetPendingQueryNodeForDevice(igtlioDevice* device);
  /// Remove the query node from the list of pending queries
  bool RemovePendingQueryNode(vtkMRMLIGTLQueryNode* queryNode);
  /// Remove queries that have timed out from the list of pending queries
  void RemoveExpiredQueries();

public:
  vtkMRMLIGTLConnectorNode* External;
  igtlioConnector* IOConnector;

  typedef std::map<std::string, igtlioConnector::NodeInfoType>   NodeInfoMapType;
  typedef std::map<std::string, vtkSmartPointer <igtlioDevice> > MessageDeviceMapType;

  // Calling StartModify() on incoming nodes when messages are received, and EndModify() once all incoming messages have been parsed is a neccesary step.
  // If a Modified() event is triggered on an incoming node while incoming messages are still being processed, it can trigger an early Render.
  // To prevent this, StartModify is called on all incoming nodes in: ProcessIncomingDeviceModifiedEvent() and EndModify() is called on modified nodes in PeriodicProcess().
  struct NodeModification
  {
    int Modifying;
    vtkSmartPointer<vtkMRMLNode> Node;
  };
  std::deque<NodeModification> PendingNodeModifications;

  typedef std::map<std::string, int> IncomingNodeClientIDMapType;
  IncomingNodeClientIDMapType IncomingNodeClientIDMap;

  NodeInfoMapType IncomingMRMLNodeInfoMap;
  MessageDeviceMapType  OutgoingMRMLIDToDeviceMap;
  MessageDeviceMapType  IncomingMRMLIDToDeviceMap;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::vtkInternal(vtkMRMLIGTLConnectorNode* external)
  : External(external)
{
  this->IOConnector = igtlioConnector::New();
}


//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkInternal::~vtkInternal()
{
  this->IOConnector->Delete();
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::AssignOutGoingNodeToDevice(vtkMRMLNode* node, igtlioDevicePointer device)
{
  this->Internal->OutgoingMRMLIDToDeviceMap[node->GetID()] = device;
  vtkIGTLToMRMLBase* converter = vtkIGTLIOConverterFactory::GetInstance()
                                   ->GetConverter(device->GetDeviceType());
  if (!converter)
  {
    return 0;
  }
  return converter->MRMLToDevice(node, device);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessNewDeviceEvent(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), void* vtkNotUsed(callData))
{
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ProcessOutgoingDeviceModifiedEvent(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice* modifiedDevice)
{
  this->IOConnector->SendMessage(igtlioDeviceKeyType::CreateDeviceKey(modifiedDevice), modifiedDevice->MESSAGE_PREFIX_NOT_DEFINED);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIncomingDeviceModifiedEvent(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), igtlioDevice* modifiedDevice)
{
  vtkMRMLNode* modifiedNode = this->GetMRMLNodeForDevice(modifiedDevice);
  bool isNewNodeCreated = false;
  if (!modifiedNode)
  {
    // Could not find node, create new node
    modifiedNode = this->CreateNewMRMLNodeForDevice(modifiedDevice);
    // new node created, notify other class about the creation of new node at the end of this function when all content are assigned to the new node.
    isNewNodeCreated = true;
  }
  if (!modifiedNode)
  {
    // Could not add node.
    return;
  }

  int wasModifyingNode = modifiedNode->StartModify();

  const std::string deviceType = modifiedDevice->GetDeviceType();
  vtkIGTLToMRMLBase* converter = vtkIGTLIOConverterFactory::GetInstance()->GetConverter(deviceType);
  if (converter)
  {
    converter->DeviceToMRML(modifiedDevice, modifiedNode, this);
  }

  vtkMRMLIGTLQueryNode* queryNode = this->Internal->GetPendingQueryNodeForDevice(modifiedDevice);
  if (queryNode && modifiedNode)
  {
    this->Internal->RemovePendingQueryNode(queryNode);
    queryNode->SetResponseDataNodeID(modifiedNode->GetID());
    queryNode->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_SUCCESS);
    queryNode->InvokeEvent(vtkMRMLIGTLQueryNode::ResponseEvent);
  }

  this->Internal->IncomingNodeClientIDMap[modifiedNode->GetName()] = modifiedDevice->GetClientID();
  modifiedNode->EndModify(wasModifyingNode);

  if(isNewNodeCreated)
  {
     //New device event needs to be forward with the MRMLNode to other objects, such as openigtlinkRemote. where the node will be processed to set name accordingly.
     this->InvokeEvent(NewDeviceEvent, modifiedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::DeviceAboutToReceiveEvent(igtlioDevice* modifiedDevice)
{
  if (!modifiedDevice)
  {
    vtkErrorWithObjectMacro(modifiedDevice, "Invalid device");
    return;
  }

  vtkMRMLNode* modifiedNode = this->External->GetMRMLNodeForDevice(modifiedDevice);
  if (!modifiedNode)
  {
    // Could not find node, create new node
    modifiedNode = this->External->CreateNewMRMLNodeForDevice(modifiedDevice);
    if (modifiedNode)
    {
      //New device event needs to be forward with the MRMLNode to other objects, such as openigtlinkRemote. where the node will be processed to set name accordingly.
      this->External->InvokeEvent(NewDeviceEvent, modifiedNode);
    }
  }
  if (!modifiedNode)
  {
    // Could not add node.
    return;
  }

  vtkInternal::NodeModification modifying;
  modifying.Node = modifiedNode;
  modifying.Modifying = modifiedNode->StartModify();
  this->PendingNodeModifications.push_back(modifying);
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetMRMLNodeForDevice(igtlioDevice* device)
{
  if (!this->GetScene())
  {
    // No scene to add nodes to.
    return nullptr;
  }

  const std::string deviceType = device->GetDeviceType();
  std::string deviceName = device->GetDeviceName();

  // Found the node and return the node;
  vtkInternal::NodeInfoMapType::iterator inIter;
  for (inIter = this->Internal->IncomingMRMLNodeInfoMap.begin();
    inIter != this->Internal->IncomingMRMLNodeInfoMap.end();
    inIter++)
  {
    vtkMRMLNode* node = this->GetScene()->GetNodeByID((inIter->first));
    if (node)
    {
      bool typeMatched = false;
      for (unsigned int i = 0; i < this->GetNodeTagFromDeviceType(deviceType.c_str()).size(); i++)
      {
        const std::string nodeTag = this->GetNodeTagFromDeviceType(deviceType.c_str())[i];
        if (strcmp(node->GetNodeTagName(), nodeTag.c_str()) == 0)
        {
          typeMatched = 1;
          break;
        }
      }
      //Node name can be modified by other module, it is not a concrete way to find the match to the device name
      // OriginalNodeNameKey arrtribute was added during node creation.
      if (typeMatched && node->GetAttribute(OriginalNodeNameKey) && strcmp(node->GetAttribute(OriginalNodeNameKey), deviceName.c_str()) == 0)
      {
        return node;
      }
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::CreateNewMRMLNodeForDevice(igtlioDevice* device)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene)
  {
    return nullptr;
  }
  vtkIGTLToMRMLBase* converter = vtkIGTLIOConverterFactory::GetInstance()
                                   ->GetConverter(device->GetDeviceType());
  if (!converter)
  {
    return nullptr;
  }
  return converter->CreateNode(scene, device, this);
}

//----------------------------------------------------------------------------
igtlioCommandPointer vtkMRMLIGTLConnectorNode::vtkInternal::SendCommand(igtlioCommandPointer command)
{
  this->IOConnector->SendCommand(command);
  return command;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::vtkInternal::SendCommandResponse(igtlioCommandPointer command)
{
  return this->IOConnector->SendCommandResponse(command);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::ReceiveCommandResponse(igtlioCommandPointer command)
{
  // Invoke command responses both on the igtlioCommand and the connectorNode.
  this->External->InvokeEvent(CommandResponseReceivedEvent, command);
}

//----------------------------------------------------------------------------
vtkMRMLIGTLQueryNode* vtkMRMLIGTLConnectorNode::vtkInternal::GetPendingQueryNodeForDevice(igtlioDevice* device)
{
  vtkMRMLIGTLQueryNode* queryNode = nullptr;
  {
    std::lock_guard<std::mutex> lock(this->External->QueryQueueMutex);
    for (QueryListType::iterator igtlQueryIt = this->External->QueryWaitingQueue.begin();
      igtlQueryIt != this->External->QueryWaitingQueue.end(); ++igtlQueryIt)
    {
      vtkMRMLIGTLQueryNode* currentQueryNode = *igtlQueryIt;
      if (currentQueryNode && currentQueryNode->GetIGTLName() == device->GetDeviceType() &&
        (!currentQueryNode->GetIGTLDeviceName() || !currentQueryNode->GetIGTLDeviceName()[0] || currentQueryNode->GetIGTLDeviceName() == device->GetDeviceName()))
      {
        queryNode = currentQueryNode;
        break;
      }
    }
  }
  return queryNode;
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::vtkInternal::RemovePendingQueryNode(vtkMRMLIGTLQueryNode* queryNode)
{
  std::lock_guard<std::mutex> lock(this->External->QueryQueueMutex);
  QueryListType::iterator queryIt = std::find(this->External->QueryWaitingQueue.begin(), this->External->QueryWaitingQueue.end(), queryNode);
  if (queryIt == this->External->QueryWaitingQueue.end())
  {
    // Could not find query to remove
    return false;
  }
  this->External->QueryWaitingQueue.erase(queryIt);
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::vtkInternal::RemoveExpiredQueries()
{
  // Queries that are no longer pending and must be dropped from the queue.
  QueryListType finishedQueries;
  // Queries that have exceeded their timeout and must additionally be marked expired.
  QueryListType expiredQueries;
  {
    std::lock_guard<std::mutex> lock(this->External->QueryQueueMutex);
    // Weak pointers are set to nullptr when the query node is deleted from the scene
    this->External->QueryWaitingQueue.remove(nullptr);
    for (QueryListType::iterator queryIt = this->External->QueryWaitingQueue.begin(); queryIt != this->External->QueryWaitingQueue.end(); ++queryIt)
    {
      vtkMRMLIGTLQueryNode* queryNode = *queryIt;
      if (!queryNode)
      {
        continue;
      }

      if (queryNode->GetQueryStatus() != vtkMRMLIGTLQueryNode::STATUS_WAITING)
      {
        // Query is not pending
        finishedQueries.push_back(queryNode);
        continue;
      }

      double timeout = queryNode->GetTimeOut();
      if (timeout <= 0.0)
      {
        // Query has no timeout
        continue;
      }

      double timeElapsed = vtkTimerLog::GetUniversalTime() - queryNode->GetTimeStamp();
      if (timeElapsed > timeout)
      {
        // Query is expired
        expiredQueries.push_back(queryNode);
        finishedQueries.push_back(queryNode);
      }
    }
  }

  // Node modifications fire Modified() and may re-enter this connector from an observer,
  // so they are done after the queue mutex has been released.
  for (QueryListType::iterator queryIt = expiredQueries.begin(); queryIt != expiredQueries.end(); ++queryIt)
  {
    vtkMRMLIGTLQueryNode* queryNode = *queryIt;
    if (queryNode)
    {
      queryNode->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_EXPIRED);
    }
  }

  for (QueryListType::iterator queryIt = finishedQueries.begin(); queryIt != finishedQueries.end(); ++queryIt)
  {
    this->RemovePendingQueryNode(*queryIt);
  }
}

//----------------------------------------------------------------------------
// vtkMRMLIGTLConnectorNode method

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::vtkMRMLIGTLConnectorNode()
{
  this->Internal = new vtkInternal(this);
  this->HideFromEditors = false;
  this->UseStreamingVolume = false;
  this->ConnectEvents();

  this->IncomingNodeReferenceRole = NULL;
  this->IncomingNodeReferenceMRMLAttributeName = NULL;
  this->OutgoingNodeReferenceRole = NULL;
  this->OutgoingNodeReferenceMRMLAttributeName = NULL;

  this->SetIncomingNodeReferenceRole("incoming");
  this->SetIncomingNodeReferenceMRMLAttributeName("incomingNodeRef");
  this->AddNodeReferenceRole(this->GetIncomingNodeReferenceRole(),
    this->GetIncomingNodeReferenceMRMLAttributeName());

  this->SetOutgoingNodeReferenceRole("outgoing");
  this->SetOutgoingNodeReferenceMRMLAttributeName("outgoingNodeRef");
  this->AddNodeReferenceRole(this->GetOutgoingNodeReferenceRole(),
    this->GetOutgoingNodeReferenceMRMLAttributeName());

  this->OutgoingMessageHeaderVersionMaximum = -1;
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode::~vtkMRMLIGTLConnectorNode()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ConnectEvents()
{
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ConnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DisconnectedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->ActivatedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->DeactivatedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->NewDeviceEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);
  this->Internal->IOConnector->AddObserver(this->Internal->IOConnector->RemovedDeviceEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandReceivedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandResponseEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
  this->Internal->IOConnector->AddObserver(igtlioCommand::CommandCompletedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetNodeTagFromDeviceType(const char* deviceType)
{
  return vtkIGTLIOConverterFactory::GetInstance()->GetNodeTagsFromDeviceType(deviceType);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkMRMLIGTLConnectorNode::GetDeviceTypeFromMRMLNodeType(const char* nodeTag)
{
  return vtkIGTLIOConverterFactory::GetInstance()->GetDeviceTypesFromNodeTag(nodeTag);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(caller);
  if (!node)
  {
    return;
  }

  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      this->PushNode(node);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents(vtkObject* caller, unsigned long event, void* callData)
{
  igtlioConnector* connector = igtlioConnector::SafeDownCast(caller);
  if (connector == NULL)
  {
    // we are only interested in proxy node modified events
    return;
  }

  int mrmlEvent = -1;
  switch (event)
  {
  case igtlioConnector::ConnectedEvent:
    mrmlEvent = ConnectedEvent;
    break;
  case igtlioConnector::DisconnectedEvent:
    mrmlEvent = DisconnectedEvent;
    break;
  case igtlioConnector::ActivatedEvent:
    mrmlEvent = ActivatedEvent;
    break;
  case igtlioConnector::DeactivatedEvent:
    mrmlEvent = DeactivatedEvent;
    break;
  case igtlioConnector::NewDeviceEvent:
    mrmlEvent = NewDeviceEvent;
    break;
  case igtlioConnector::RemovedDeviceEvent:
    mrmlEvent = DeviceModifiedEvent;
    break;
  }

  if (mrmlEvent == ConnectedEvent)
  {
    vtkInfoMacro("Connected: " << connector->GetServerHostname() << ":" << connector->GetServerPort());

    // Slicer wants to make use of meta data in IGTL messages, so tell the server we can support v2 messages
    igtlioDeviceKeyType key;
    key.name = "";
    key.type = "STATUS";
    vtkSmartPointer<igtlioStatusDevice> statusDevice = igtlioStatusDevice::SafeDownCast(this->Internal->IOConnector->GetDevice(key));
    if (!statusDevice)
    {
      statusDevice = vtkSmartPointer<igtlioStatusDevice>::New();
      connector->AddDevice(statusDevice);
    }

    if (this->OutgoingMessageHeaderVersionMaximum < 0 || this->OutgoingMessageHeaderVersionMaximum >= IGTL_HEADER_VERSION_2)
    {
      statusDevice->SetMetaDataElement("dummy", "dummy"); // existence of metadata makes the IO connector send a header v2 message
    }
    connector->SendMessage(igtlioDeviceKeyType::CreateDeviceKey(statusDevice));
    connector->RemoveDevice(statusDevice);

    this->PushOnConnect();
  }
  else if (mrmlEvent == DisconnectedEvent)
  {
    vtkInfoMacro("Disconnected: " << connector->GetServerHostname() << ":" << connector->GetServerPort());
  }
  else if (event == igtlioConnector::NewDeviceEvent)
  {
    igtlioDevice* modifiedDevice = static_cast<igtlioDevice*>(callData);
    if (modifiedDevice)
    {
      // no action perform at this stage, wait until the message content in the device is unpacked,
      // As we need the message content data to create mrmlnode.
      modifiedDevice->AddObserver(modifiedDevice->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
      modifiedDevice->AddObserver(igtlioDevice::AboutToReceiveEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);
    }
  }

  if (mrmlEvent > 0)
  {
    this->InvokeEvent(mrmlEvent, callData);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PushOnConnect()
{
  for (int i = 0; i < this->GetNumberOfOutgoingMRMLNodes(); ++i)
  {
    vtkMRMLNode* node = this->GetOutgoingMRMLNode(i);
    if (!node)
    {
      vtkErrorMacro("PushOnConnect: Could not find outgoing node!");
      continue;
    }
    const char* pushOnConnect = node->GetAttribute("OpenIGTLinkIF.pushOnConnect");
    if (pushOnConnect && strcmp(pushOnConnect, "true") == 0)
    {
      this->PushNode(node);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents(vtkObject* caller, unsigned long event, void* callData)
{
  igtlioDevice* modifiedDevice = igtlioDevice::SafeDownCast(caller);
  if (modifiedDevice == NULL)
  {
    return;
  }

  int mrmlEvent = -1;
  if (event == igtlioDevice::AboutToReceiveEvent)
  {
    this->Internal->DeviceAboutToReceiveEvent(modifiedDevice);
  }
  else if (event == modifiedDevice->GetDeviceContentModifiedEvent())
  {
    mrmlEvent = DeviceModifiedEvent;
    if (modifiedDevice->MessageDirectionIsIn())
    {
      this->ProcessIncomingDeviceModifiedEvent(caller, event, modifiedDevice);
    }
  }

  if (mrmlEvent > 0)
  {
    this->InvokeEvent(mrmlEvent, modifiedDevice);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ProcessIOCommandEvents(vtkObject* caller, unsigned long event, void* callData)
{
  igtlioConnector* connector = reinterpret_cast<igtlioConnector*>(caller);
  if (connector == NULL)
  {
    // we are only interested in proxy node modified events
    return;
  }

  int mrmlEvent = -1;
  switch (event)
  {
  case igtlioCommand::CommandReceivedEvent:
    mrmlEvent = CommandReceivedEvent;
    break;  // COMMAND received
  case igtlioCommand::CommandResponseEvent:
    mrmlEvent = CommandResponseReceivedEvent;
    break;  // RTS_COMMAND received
  case igtlioCommand::CommandCompletedEvent:
    mrmlEvent = CommandCompletedEvent;
    break;  // Sent COMMAND did not receive a response before timeout
  }

  igtlioCommand* command = static_cast<igtlioCommand*>(callData);
  if (command)
  {
    vtkNew<vtkSlicerOpenIGTLinkCommand> slicerCommand;
    slicerCommand->SetCommand(command);
    this->InvokeEvent(mrmlEvent, slicerCommand);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::WriteXML(ostream& of, int nIndent)
{

  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);

  switch (this->Internal->IOConnector->GetType())
  {
  case igtlioConnector::TYPE_SERVER:
    of << " connectorType=\"" << "SERVER" << "\" ";
    break;
  case igtlioConnector::TYPE_CLIENT:
    of << " connectorType=\"" << "CLIENT" << "\" ";
    of << " serverHostname=\"" << this->Internal->IOConnector->GetServerHostname() << "\" ";
    break;
  default:
    of << " connectorType=\"" << "NOT_DEFINED" << "\" ";
    break;
  }

  of << " serverPort=\"" << this->Internal->IOConnector->GetServerPort() << "\" ";
  of << " persistent=\"" << this->Internal->IOConnector->GetPersistent() << "\" ";
  of << " checkCRC=\"" << this->Internal->IOConnector->GetCheckCRC() << "\" ";
  of << " state=\"" << this->Internal->IOConnector->GetState() << "\"";
  of << " restrictDeviceName=\"" << this->Internal->IOConnector->GetRestrictDeviceName() << "\" ";
  if (this->OutgoingMessageHeaderVersionMaximum > 0)
  {
    of << " outgoingMessageHeaderVersionMaximum=\"" << this->GetOutgoingMessageHeaderVersionMaximum() << "\" ";
  }
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;

  const char* serverHostname = "";
  int port = 0;
  int type = -1;
  int restrictDeviceName = 0;
  int state = igtlioConnector::STATE_OFF;
  int persistent = igtlioConnector::PERSISTENT_OFF;

  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "connectorType"))
    {
      if (!strcmp(attValue, "SERVER"))
      {
        type = igtlioConnector::TYPE_SERVER;
      }
      else if (!strcmp(attValue, "CLIENT"))
      {
        type = igtlioConnector::TYPE_CLIENT;
      }
      else
      {
        type = igtlioConnector::TYPE_NOT_DEFINED;
      }
    }
    if (!strcmp(attName, "serverHostname"))
    {
      serverHostname = attValue;
    }
    if (!strcmp(attName, "serverPort"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> port;
    }
    if (!strcmp(attName, "restrictDeviceName"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> restrictDeviceName;;
    }
    if (!strcmp(attName, "persistent"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> persistent;
    }
    if (!strcmp(attName, "checkCRC"))
    {
      std::stringstream ss;
      ss << attValue;
      bool checkCRC = true;
      ss >> checkCRC;
      this->SetCheckCRC(checkCRC);
    }
    if (!strcmp(attName, "state"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> state;
    }
    if (!strcmp(attName, "outgoingMessageHeaderVersionMaximum"))
    {
      std::stringstream ss;
      ss << attValue;
      int outgoingMessageHeaderVersionMaximum = -1;
      ss >> outgoingMessageHeaderVersionMaximum;
      this->SetOutgoingMessageHeaderVersionMaximum(outgoingMessageHeaderVersionMaximum);
    }

    switch (type)
    {
    case igtlioConnector::TYPE_SERVER:
      this->Internal->IOConnector->SetTypeServer(port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    case igtlioConnector::TYPE_CLIENT:
      this->Internal->IOConnector->SetTypeClient(serverHostname, port);
      this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
      break;
    default: // not defined
      // do nothing
      break;
    }

    if (persistent == igtlioConnector::PERSISTENT_ON)
    {
      this->Internal->IOConnector->SetPersistent(igtlioConnector::PERSISTENT_ON);
      // At this point not all the nodes are loaded so we cannot start
      // the processing thread (if we activate the connection then it may immediately
      // start receiving messages, create corresponding MRML nodes, while the same
      // MRML nodes are being loaded from the scene; this would result in duplicate
      // MRML nodes).
      // All the nodes will be activated by the module logic when the scene import is finished.
      this->Internal->IOConnector->SetState(state);
      this->Modified();
    }
  }
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLIGTLConnectorNode::Copy(vtkMRMLNode* anode)
{
  Superclass::Copy(anode);
  vtkMRMLIGTLConnectorNode* node = (vtkMRMLIGTLConnectorNode*)anode;

  int type = node->Internal->IOConnector->GetType();

  switch (type)
  {
  case igtlioConnector::TYPE_SERVER:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_SERVER);
    this->Internal->IOConnector->SetTypeServer(node->Internal->IOConnector->GetServerPort());
    this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
    break;
  case igtlioConnector::TYPE_CLIENT:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_CLIENT);
    this->Internal->IOConnector->SetTypeClient(node->Internal->IOConnector->GetServerHostname(), node->Internal->IOConnector->GetServerPort());
    this->Internal->IOConnector->SetRestrictDeviceName(node->Internal->IOConnector->GetRestrictDeviceName());
    break;
  default: // not defined
    // do nothing
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_NOT_DEFINED);
    break;
  }
  this->Internal->IOConnector->SetPersistent(node->Internal->IOConnector->GetPersistent());
  this->SetCheckCRC(node->GetCheckCRC());
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  if (this->Internal->IOConnector->GetType() == igtlioConnector::TYPE_SERVER)
  {
    os << indent << "Connector Type : SERVER\n";
    os << indent << "Listening Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
  }
  else if (this->Internal->IOConnector->GetType() == igtlioConnector::TYPE_CLIENT)
  {
    os << indent << "Connector Type: CLIENT\n";
    os << indent << "Server Hostname: " << this->Internal->IOConnector->GetServerHostname() << "\n";
    os << indent << "Server Port #: " << this->Internal->IOConnector->GetServerPort() << "\n";
  }

  switch (this->Internal->IOConnector->GetState())
  {
  case igtlioConnector::STATE_OFF:
    os << indent << "State: OFF\n";
    break;
  case igtlioConnector::STATE_WAIT_CONNECTION:
    os << indent << "State: WAIT FOR CONNECTION\n";
    break;
  case igtlioConnector::STATE_CONNECTED:
    os << indent << "State: CONNECTED\n";
    break;
  }
  os << indent << "Persistent: " << this->Internal->IOConnector->GetPersistent() << "\n";
  os << indent << "Restrict Device Name: " << this->Internal->IOConnector->GetRestrictDeviceName() << "\n";
  os << indent << "Push Outgoing Message Flag: " << this->Internal->IOConnector->GetPushOutgoingMessageFlag() << "\n";
  os << indent << "Check CRC: " << this->GetCheckCRC() << "\n";
  os << indent << "Number of outgoing nodes: " << this->GetNumberOfOutgoingMRMLNodes() << "\n";
  os << indent << "Number of incoming nodes: " << this->GetNumberOfIncomingMRMLNodes() << "\n";
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceAdded(vtkMRMLNodeReference* reference)
{
  vtkMRMLScene* scene = this->GetScene();
  if (!scene)
  {
    return;
  }

  vtkMRMLNode* node = scene->GetNodeByID(reference->GetReferencedNodeID());
  if (!node)
  {
    return;
  }

  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
  {
    // Add new NodeInfoType structure
    igtlioConnector::NodeInfoType nodeInfo;
    //nodeInfo.node = node;
    nodeInfo.lock = 0;
    nodeInfo.second = 0;
    nodeInfo.nanosecond = 0;
    this->Internal->IncomingMRMLNodeInfoMap[node->GetID()] = nodeInfo;
  }
  else if (strcmp(reference->GetReferenceRole(), this->GetOutgoingNodeReferenceRole()) == 0)
  {
    // Find a converter for the node
    igtlioDevicePointer device = NULL;
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
    if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
    {
      igtlioDeviceKeyType key;
      key.name = node->GetName();
      std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
      if (deviceTypes.empty())
      {
        vtkErrorMacro("Outgoing node reference is not supported for node type: " << node->GetClassName());
      }

      for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
      {
        key.type = deviceTypes[typeIndex];
        device = this->Internal->IOConnector->GetDevice(key);
        if (device != NULL)
        {
          break;
        }
      }
      if (device == NULL)
      {
        //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO" for ScalarVolumeNode.
        igtlioDeviceFactoryPointer factory = this->Internal->IOConnector->GetDeviceFactory();
        if (factory && !deviceTypes.empty())
        {
          device = factory->create(deviceTypes[0], key.name);
        }
        if (device)
        {
          device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
          this->Internal->OutgoingMRMLIDToDeviceMap[node->GetID()] = device;
          this->Internal->IOConnector->AddDevice(device);
        }
      }
    }
    else
    {
      device = this->Internal->OutgoingMRMLIDToDeviceMap[node->GetID()];
    }
    if (!device)
    {
      // Could not create outgoing node. Remove the node reference
      int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
      for (int i = 0; i < n; i++)
      {
        const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
        if (strcmp(node->GetID(), id) == 0)
        {
          this->RemoveNthNodeReferenceID(this->OutgoingNodeReferenceRole, i);
          break;
        }
      }
      return;
    }
    device->SetMessageDirection(igtlioDevice::MESSAGE_DIRECTION_OUT);
    unsigned int nodeModifiedEvent = this->AssignOutGoingNodeToDevice(node, device);
    node->AddObserver(nodeModifiedEvent, this, &vtkMRMLIGTLConnectorNode::ProcessIOConnectorEvents);

    // Need to update the events here because observed events are not saved in the scene
    // for each reference and therefore only the role-default event observers are added.
    // Get the correct list of events to observe from the converter and update the reference
    // with that.
    // Without doing this, outgoing transforms are not updated if connectors are set up from
    // a saved scene.
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i++)
    {
      const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
      {
        vtkSmartPointer<vtkIntArray> nodeEvents = vtkSmartPointer<vtkIntArray>::New();
        nodeEvents->InsertNextValue(nodeModifiedEvent);
        this->SetAndObserveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i,
          node->GetID(), nodeEvents);
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::OnNodeReferenceRemoved(vtkMRMLNodeReference* reference)
{
  const char* nodeID = reference->GetReferencedNodeID();
  if (!nodeID)
  {
    return;
  }
  if (strcmp(reference->GetReferenceRole(), this->GetIncomingNodeReferenceRole()) == 0)
  {
    // Check if the node has already been reagistered.
    // TODO: MRMLNodeListType can be reimplemented as a std::list
    // so that the converter can be removed by 'remove()' method.
    vtkInternal::NodeInfoMapType::iterator iter;
    iter = this->Internal->IncomingMRMLNodeInfoMap.find(nodeID);
    if (iter != this->Internal->IncomingMRMLNodeInfoMap.end())
    {
      this->Internal->IncomingMRMLNodeInfoMap.erase(iter);
    }
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(nodeID);
    if (citer != this->Internal->IncomingMRMLIDToDeviceMap.end())
    {
      this->Internal->IncomingMRMLIDToDeviceMap.erase(citer);
    }
    else
    {
      vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: " << nodeID);
    }
  }
  else
  {
    // Search converter from OutgoingMRMLIDToDeviceMap
    vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(nodeID);
    if (citer != this->Internal->OutgoingMRMLIDToDeviceMap.end())
    {
      vtkSmartPointer<igtlioDevice> device = citer->second;
      device->RemoveObserver(device->GetDeviceContentModifiedEvent());
      this->Internal->IOConnector->RemoveDevice(device);
      this->Internal->OutgoingMRMLIDToDeviceMap.erase(citer);
    }
    else
    {
      vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: " << nodeID);
    }
  }
}


//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
  {
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return false;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return false;
  }

  igtlioDevicePointer device = NULL;
  igtlioDeviceKeyType key;
  key.name = node->GetName();
  std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
  for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
  {
    key.type = deviceTypes[typeIndex];
    device = this->Internal->IOConnector->GetDevice(key);
    if (device != NULL)
    {
      break;
    }
  }

  if (device == NULL)
  {
    device = this->Internal->IOConnector->GetDeviceFactory()->create(key.type, key.name);
  }

  return this->RegisterIncomingMRMLNode(node, device);
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::RegisterIncomingMRMLNode(vtkMRMLNode* node, IGTLDevicePointer device)
{
  if (!node)
  {
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return false;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return false;
  }

  igtlioDevice* igtlDevice = static_cast<igtlioDevice*>(device);

  // Check if the node has already been registered.
  if (this->HasNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID()))
  {
    // the node has been already registered.
    return false;
  }
  else
  {
    this->Internal->IncomingMRMLIDToDeviceMap[node->GetID()] = igtlDevice;
    this->AddAndObserveNodeReferenceID(this->GetIncomingNodeReferenceRole(), node->GetID());
    this->Modified();
  }
  node->SetAttribute(OriginalNodeNameKey, node->GetName());
  return true;

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterIncomingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
  {
    vtkErrorMacro("Error registering incoming node. Incoming node is NULL");
    return;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering incoming node. Node scene is NULL");
    return;
  }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Already on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i);
      vtkInternal::NodeInfoMapType::iterator iter;
      iter = this->Internal->IncomingMRMLNodeInfoMap.find(id);
      if (iter != this->Internal->IncomingMRMLNodeInfoMap.end())
      {
        this->Internal->IncomingMRMLNodeInfoMap.erase(iter);
      }
      vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(node->GetID());
      if (citer != this->Internal->IncomingMRMLIDToDeviceMap.end())
      {
        this->Internal->IncomingMRMLIDToDeviceMap.erase(citer);
      }
      else
      {
        vtkErrorMacro("Node is not found in IncomingMRMLIDToDeviceMap: " << node->GetID());
      }
      this->Modified();
      break;
    }
  }

}

//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfOutgoingMRMLNodes()
{
  //return this->OutgoingMRMLNodeList.size();
  return this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetOutgoingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole()))
  {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
    {
      return NULL;
    }
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i));
    return node;
  }
  else
  {
    return NULL;
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType)
{
  if (!node)
  {
    vtkErrorMacro("Error registering outgoing node. Incoming node is NULL");
    return 0;
  }

  if (!node->GetScene())
  {
    vtkErrorMacro("Error registering outgoing node. Node scene is NULL");
    return 0;
  }

  // TODO: Need to check the existing device type?
  if (node->GetAttribute("OpenIGTLinkIF.out.type") == NULL)
  {
    node->SetAttribute("OpenIGTLinkIF.out.type", devType);
  }
  if (node->GetAttribute("OpenIGTLinkIF.out.name") == NULL)
  {
    node->SetAttribute("OpenIGTLinkIF.out.name", node->GetName());
  }



  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      break;
    }
  }

  if (this->AddAndObserveNodeReferenceID(this->GetOutgoingNodeReferenceRole(), node->GetID()))
  {
    igtlioDevicePointer device = NULL;
    igtlioDeviceKeyType key;
    key.name = node->GetName();
    std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(node->GetNodeTagName());
    for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
    {
      key.type = deviceTypes[typeIndex];
      device = this->Internal->IOConnector->GetDevice(key);
      if (device != NULL)
      {
        break;
      }
    }
    this->Modified();

    return 1;
  }
  else // If the reference node wasn't associated with any device, delete the reference
  {
    int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
    for (int i = 0; i < n; i++)
    {
      const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      if (strcmp(node->GetID(), id) == 0)
      {
        // Alredy on the list. Remove it.
        this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
        break;
      }
    }
    return 0;
  }

}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnregisterOutgoingMRMLNode(vtkMRMLNode* node)
{
  if (!node)
  {
    return;
  }

  // Check if the node is on the reference list for outgoing nodes
  int n = this->GetNumberOfNodeReferences(this->GetOutgoingNodeReferenceRole());
  for (int i = 0; i < n; i++)
  {
    const char* id = this->GetNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
    if (strcmp(node->GetID(), id) == 0)
    {
      // Alredy on the list. Remove it.
      this->RemoveNthNodeReferenceID(this->GetOutgoingNodeReferenceRole(), i);
      this->Modified();
      break;
    }
  }

}

//---------------------------------------------------------------------------
unsigned int vtkMRMLIGTLConnectorNode::GetNumberOfIncomingMRMLNodes()
{
  //return this->IncomingMRMLNodeInfoMap.size();
  return this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole());
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLIGTLConnectorNode::GetIncomingMRMLNode(unsigned int i)
{
  if (i < (unsigned int)this->GetNumberOfNodeReferences(this->GetIncomingNodeReferenceRole()))
  {
    vtkMRMLScene* scene = this->GetScene();
    if (!scene)
    {
      return NULL;
    }
    vtkMRMLNode* node = scene->GetNodeByID(this->GetNthNodeReferenceID(this->GetIncomingNodeReferenceRole(), i));
    return node;
  }
  else
  {
    return NULL;
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::PushNode(vtkMRMLNode* node)
{
  if (!node)
  {
    return 0;
  }

  if (!(node->GetID()))
  {
    return 0;
  }

  if (node->IsA("vtkMRMLIGTLQueryNode"))
  {
    return this->PushQuery(vtkMRMLIGTLQueryNode::SafeDownCast(node));
  }

  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(node->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    vtkErrorMacro("Node is not found in OutgoingMRMLIDToDeviceMap: " << node->GetID());
    return 0;
  }

  vtkSmartPointer<igtlioDevice> device = iter->second;
  igtlioDeviceKeyType key;
  key.name = device->GetDeviceName();
  key.type = device->GetDeviceType();
  vtkIGTLToMRMLBase* converter = vtkIGTLIOConverterFactory::GetInstance()->GetConverter(key.type);
  if (!converter)
  {
    vtkErrorMacro("PushNode: no converter registered for device type " << key.type << ", node " << node->GetID());
    return 0;
  }

  device->ClearMetaData();
  if (this->OutgoingMessageHeaderVersionMaximum < 0 || this->OutgoingMessageHeaderVersionMaximum >= IGTL_HEADER_VERSION_2)
  {
    converter->PrepareOutgoingDevice(node, device);
  }

  device->RemoveObservers(device->GetDeviceContentModifiedEvent());
  converter->MRMLToDevice(node, device); // update the device content
  device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLIGTLConnectorNode::ProcessIODeviceEvents);


  int incomingClientID = -1;
  vtkInternal::IncomingNodeClientIDMapType::iterator incomingClientIDIt = this->Internal->IncomingNodeClientIDMap.find(node->GetName());
  if (incomingClientIDIt != this->Internal->IncomingNodeClientIDMap.end())
  {
    incomingClientID = incomingClientIDIt->second;
  }

  // Send the node to all connected clients
  std::vector<int> clientIDs = this->Internal->IOConnector->GetClientIds();
  for (std::vector<int>::iterator clientIDIt = clientIDs.begin(); clientIDIt != clientIDs.end(); ++clientIDIt)
  {
    int clientID = *clientIDIt;
    if (clientID == incomingClientID)
    {
      // The message was originally received from this client.
      // We don't need to send it back.
      continue;
    }

    this->Internal->IOConnector->SendMessage(key, igtlioDevice::MESSAGE_PREFIX_NOT_DEFINED, clientID);
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::PushQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node == NULL)
  {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: invalid input node");
    return 1;
  }

  igtlioDeviceKeyType key;
  key.name = node->GetIGTLDeviceName();
  key.type = node->GetIGTLName();

  if (this->Internal->IOConnector->GetDevice(key) == NULL)
  {
    igtlioDeviceCreatorPointer deviceCreator = this->Internal->IOConnector->GetDeviceFactory()->GetCreator(key.GetBaseTypeName());
    if (deviceCreator.GetPointer())
    {
      this->Internal->IOConnector->AddDevice(deviceCreator->Create(key.name));
    }
    else
    {
      vtkErrorMacro("vtkMRMLIGTLConnectorNode::PushQuery failed: Device type not supported");
      return 1;
    }
  }

  igtlioDevice::MESSAGE_PREFIX prefix = igtlioDevice::MESSAGE_PREFIX_NOT_DEFINED;
  int nodeQueryType = node->GetQueryType();
  if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_GET)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_GET;
  }
  else if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_START)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_START;
  }
  else if (nodeQueryType == vtkMRMLIGTLQueryNode::TYPE_STOP)
  {
    prefix = igtlioDevice::MESSAGE_PREFIX_STOP;
  }

  this->Internal->IOConnector->SendMessage(key, prefix);
  {
    std::lock_guard<std::mutex> lock(this->QueryQueueMutex);
    this->QueryWaitingQueue.push_back(node);
  }
  // Node modifications fire Modified() and may re-enter this connector from an observer,
  // so they are done after the queue mutex has been released.
  node->SetTimeStamp(vtkTimerLog::GetUniversalTime());
  node->SetQueryStatus(vtkMRMLIGTLQueryNode::STATUS_WAITING);
  node->SetConnectorNodeID(this->GetID());
  return 0;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelCommand(igtlioCommandPointer command)
{
  this->Internal->IOConnector->CancelCommand(command);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelCommand(int commandId, int clientId)
{
  this->Internal->IOConnector->CancelCommand(commandId, clientId);
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::CancelQuery(vtkMRMLIGTLQueryNode* node)
{
  if (node == NULL)
  {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::CancelQuery failed: invalid input node");
    return;
  }
  {
    std::lock_guard<std::mutex> lock(this->QueryQueueMutex);
    this->QueryWaitingQueue.remove(node);
  }
  // Modify the node after the queue mutex has been released (see PushQuery).
  node->SetConnectorNodeID("");
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::LockIncomingMRMLNode(vtkMRMLNode* node)
{
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
  {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
    {
      (iter->second).lock = 1;
    }
  }
}


//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::UnlockIncomingMRMLNode(vtkMRMLNode* node)
{
  // Check if the node has already been added in the locked node list
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
  {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
    {
      (iter->second).lock = 0;
      return;
    }
  }
}


//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond)
{
  // Check if the node has already been added in the locked node list
  vtkInternal::NodeInfoMapType::iterator iter;
  for (iter = this->Internal->IncomingMRMLNodeInfoMap.begin(); iter != this->Internal->IncomingMRMLNodeInfoMap.end(); iter++)
  {
    //if ((iter->second).node == node)
    if (iter->first.compare(node->GetID()) == 0)
    {
      second = (iter->second).second;
      nanosecond = (iter->second).nanosecond;
      return 1;
    }
  }
  return 0;

}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetState()
{
  if (!this->Internal->IOConnector)
  {
    return StateOff;
  }

  switch (this->Internal->IOConnector->GetState())
  {
  case igtlioConnector::STATE_OFF:
    return StateOff;
  case igtlioConnector::STATE_WAIT_CONNECTION:
    return StateWaitConnection;
  case igtlioConnector::STATE_CONNECTED:
    return StateConnected;
  default:
    return StateOff;
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetPersistent()
{
  return this->Internal->IOConnector->GetPersistent() == igtlioConnector::PERSISTENT_ON;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetPersistent(bool persistent)
{
  this->Internal->IOConnector->SetPersistent(persistent ? igtlioConnector::PERSISTENT_ON : igtlioConnector::PERSISTENT_OFF);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommand(igtlioCommandPointer command)
{
  if (!command)
  {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::SendCommand failed: Invalid command");
    return;
  }

  if (command->IsInProgress())
  {
    vtkErrorMacro("vtkMRMLIGTLConnectorNode::SendCommand failed: Command is already in progress!");
    return;
  }

  this->Internal->SendCommand(command);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommand(vtkSlicerOpenIGTLinkCommand* command)
{
  this->SendCommand(command->GetCommand());
}

//---------------------------------------------------------------------------
igtlioCommandPointer vtkMRMLIGTLConnectorNode::SendCommand(std::string name, std::string content,
  bool blocking/*=true*/, double timeout_s/*=5*/, igtl::MessageBase::MetaDataMap* metaData/*=NULL*/, int clientId/*=-1*/)
{
  igtlioCommandPointer command = igtlioCommandPointer::New();
  command->SetClientId(clientId);
  command->SetName(name);
  command->SetCommandContent(content);
  if (metaData)
  {
    command->SetCommandMetaData(*metaData);
  }
  command->SetBlocking(blocking);
  command->SetTimeoutSec(timeout_s);
  return this->Internal->SendCommand(command);
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SendCommandResponse(igtlioCommandPointer command)
{
  return this->Internal->SendCommandResponse(command);
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SendCommandResponse(vtkSlicerOpenIGTLinkCommand* command)
{
  this->SendCommandResponse(command->GetCommand());
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeServer(int port)
{
  return this->Internal->IOConnector->SetTypeServer(port);
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::SetTypeClient(std::string hostname, int port)
{
  return this->Internal->IOConnector->SetTypeClient(hostname, port);
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Start()
{
  int status = this->Internal->IOConnector->Start();
  this->Modified();
  return status;
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::Stop()
{
  int status = this->Internal->IOConnector->Stop();
  this->Modified();
  return status;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::PeriodicProcess()
{
  this->Internal->IOConnector->PeriodicProcess();

  while (!this->Internal->PendingNodeModifications.empty())
  {
    vtkInternal::NodeModification wasModifying = this->Internal->PendingNodeModifications.back();
    wasModifying.Node->EndModify(wasModifying.Modifying);
    if (wasModifying.Node->GetName())
    {
      this->Internal->IncomingNodeClientIDMap[wasModifying.Node->GetName()] = -1;
    }
    this->Internal->PendingNodeModifications.pop_back();
  }

  this->Internal->RemoveExpiredQueries();
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::AddDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->AddDevice(static_cast<igtlioDevice*>(device));
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::RemoveDevice(IGTLDevicePointer device)
{
  return this->Internal->IOConnector->RemoveDevice(static_cast<igtlioDevice*>(device));
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetRestrictDeviceName(int restrictDeviceName)
{
  this->Internal->IOConnector->SetRestrictDeviceName(restrictDeviceName);
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::GetDeviceFromOutgoingMRMLNode(const char* outgoingNodeID)
{
  vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->OutgoingMRMLIDToDeviceMap.find(outgoingNodeID);
  if (citer == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    return NULL;
  }
  return citer->second;
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::GetDeviceFromIncomingMRMLNode(const char* incomingNodeID)
{
  vtkInternal::MessageDeviceMapType::iterator citer = this->Internal->IncomingMRMLIDToDeviceMap.find(incomingNodeID);
  if (citer == this->Internal->IncomingMRMLIDToDeviceMap.end())
  {
    return NULL;
  }
  return citer->second;
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkMRMLIGTLConnectorNode::CreateDeviceForOutgoingMRMLNode(vtkMRMLNode* dnode)
{
  vtkSmartPointer<igtlioDevice> device = NULL;
  vtkInternal::MessageDeviceMapType::iterator iter = this->Internal->OutgoingMRMLIDToDeviceMap.find(dnode->GetID());
  if (iter == this->Internal->OutgoingMRMLIDToDeviceMap.end())
  {
    igtlioDeviceKeyType key;
    key.name = dnode->GetName();
    std::vector<std::string> deviceTypes = this->GetDeviceTypeFromMRMLNodeType(dnode->GetNodeTagName());
    if (deviceTypes.empty())
    {
      vtkErrorMacro("CreateDeviceForOutgoingMRMLNode: no converter registered for node type "
        << dnode->GetNodeTagName() << " (node " << (dnode->GetID() ? dnode->GetID() : "") << ")");
      return nullptr;
    }
    for (size_t typeIndex = 0; typeIndex < deviceTypes.size(); typeIndex++)
    {
      key.type = deviceTypes[typeIndex];
      device = this->Internal->IOConnector->GetDevice(key);
      if (device != NULL)
      {
        break;
      }
    }
    if (device == NULL)
    {
      //If no device is found, add a device using the first device type, such as prefer "IMAGE" over "VIDEO" for ScalarVolumeNode.
      //"VIDEO" over "IMAGE" for Streaming volume node
      key.type = deviceTypes[0];
      device = this->Internal->IOConnector->GetDeviceFactory()->create(key.type, key.name);
      if (device)
      {
        this->Internal->OutgoingMRMLIDToDeviceMap[dnode->GetID()] = device;
        this->Internal->IOConnector->AddDevice(device);
      }
    }
  }
  else
  {
    device = this->Internal->OutgoingMRMLIDToDeviceMap[dnode->GetID()];
  }
  return device;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetType(int type)
{
  switch (type)
  {
  case TypeServer:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_SERVER);
    break;
  case TypeClient:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_CLIENT);
    break;
  default:
    this->Internal->IOConnector->SetType(igtlioConnector::TYPE_NOT_DEFINED);
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetType()
{
  switch (this->Internal->IOConnector->GetType())
  {
  case igtlioConnector::TYPE_SERVER:
    return TypeServer;
  case igtlioConnector::TYPE_CLIENT:
    return TypeClient;
  default:
    return TypeNotDefined;
  }
}

//---------------------------------------------------------------------------
int vtkMRMLIGTLConnectorNode::GetServerPort()
{
  return this->Internal->IOConnector->GetServerPort();
}

//---------------------------------------------------------------------------
const char* vtkMRMLIGTLConnectorNode::GetServerHostname()
{
  return this->Internal->IOConnector->GetServerHostname();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerHostname(std::string hostname)
{
  this->Internal->IOConnector->SetServerHostname(hostname);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetUseStreamingVolume(bool useStreamingVolume)
{
  this->UseStreamingVolume = useStreamingVolume;
}
//---------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetUseStreamingVolume()
{
  return this->UseStreamingVolume;
}

//---------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetServerPort(int port)
{
  this->Internal->IOConnector->SetServerPort(port);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorNode::SetCheckCRC(bool check)
{
  this->Internal->IOConnector->SetCheckCRC(check ? 1 : 0);
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorNode::GetCheckCRC()
{
  return (this->Internal->IOConnector->GetCheckCRC() != 0);
}

#pragma pop_macro("SendMessage")
