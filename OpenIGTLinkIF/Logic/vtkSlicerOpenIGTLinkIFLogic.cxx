/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkSlicerOpenIGTLinkIFLogic.cxx $
  Date:      $Date: 2011-06-12 14:55:20 -0400 (Sun, 12 Jun 2011) $
  Version:   $Revision: 16985 $

==========================================================================*/

// Slicer includes
#include <vtkObjectFactory.h>
#include <vtkSlicerColorLogic.h>

// OpenIGTLinkIF MRML includes
#include "vtkIGTLCircularBuffer.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLImageMetaListNode.h"
#include "vtkMRMLLabelMetaListNode.h"
#include "vtkMRMLTextNode.h"
#include "vtkMRMLIGTLTrackingDataQueryNode.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLStatusNode.h"
#include "vtkMRMLIGTLSensorNode.h"

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  #include "vtkMRMLBitStreamNode.h"
#endif

// OpenIGTLinkIO Device includes
#include "igtlioDevice.h"
#include "igtlioConnector.h"
#include "igtlioDeviceFactory.h"

// OpenIGTLink includes
#include <igtlImageMessage.h>
#include <igtlTransformMessage.h>

// OpenIGTLinkIF Logic includes
#include "vtkSlicerOpenIGTLinkIFLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkTransform.h>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkIFLogic);




//---------------------------------------------------------------------------
class vtkSlicerOpenIGTLinkIFLogic::vtkInternal
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkSlicerOpenIGTLinkIFLogic* external);
  ~vtkInternal();

  vtkSlicerOpenIGTLinkIFLogic* External;

  MessageDeviceListType      MessageDeviceList;
  igtlio::DeviceFactoryPointer DeviceFactory;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::vtkInternal::vtkInternal(vtkSlicerOpenIGTLinkIFLogic* external)
  : External(external)
{
  this->DeviceFactory = igtlio::DeviceFactoryPointer::New();
}

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::vtkInternal::~vtkInternal()
{
}

//----------------------------------------------------------------------------
// vtkSlicerOpenIGTLinkIFLogic methods

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::vtkSlicerOpenIGTLinkIFLogic()
{
  this->Internal = new vtkInternal(this);

  // Timer Handling
  this->DataCallbackCommand = vtkCallbackCommand::New();
  this->DataCallbackCommand->SetClientData( reinterpret_cast<void *> (this) );
  this->DataCallbackCommand->SetCallback(vtkSlicerOpenIGTLinkIFLogic::DataCallback);

  this->Initialized   = 0;
  this->RestrictDeviceName = 0;
  
  std::vector<std::string> deviceTypes = this->Internal->DeviceFactory->GetAvailableDeviceTypes();
  for (int typeIndex = 0; typeIndex<deviceTypes.size();typeIndex++)
  {
    this->Internal->MessageDeviceList.push_back(this->Internal->DeviceFactory->GetCreator(deviceTypes[typeIndex])->Create(""));
  }
    
  //this->LocatorTransformNode = NULL;
}

//---------------------------------------------------------------------------
vtkSlicerOpenIGTLinkIFLogic::~vtkSlicerOpenIGTLinkIFLogic()
{
  if (this->DataCallbackCommand)
    {
    this->DataCallbackCommand->Delete();
    }
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkSlicerOpenIGTLinkIFLogic:             " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> sceneEvents;
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::RegisterNodes()
{
  vtkMRMLScene * scene = this->GetMRMLScene();
  if(!scene)
    {
    return;
    }

  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLConnectorNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLImageMetaListNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLLabelMetaListNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLTextNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLTrackingDataQueryNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLTrackingDataBundleNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLStatusNode>().GetPointer());
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLSensorNode>().GetPointer());
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  scene->RegisterNodeClass(vtkNew<vtkMRMLBitStreamNode>().GetPointer());
#endif
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::DataCallback(vtkObject *vtkNotUsed(caller),
                                               unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkSlicerOpenIGTLinkIFLogic *self = reinterpret_cast<vtkSlicerOpenIGTLinkIFLogic *>(clientData);
  vtkDebugWithObjectMacro(self, "In vtkSlicerOpenIGTLinkIFLogic DataCallback");
  self->UpdateAll();
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::UpdateAll()
{
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return;
    }
  // Make sure we don't add duplicate observation
  vtkUnObserveMRMLNodeMacro(connectorNode);
  // Start observing the connector node
  vtkNew<vtkIntArray> connectorNodeEvents;
  connectorNodeEvents->InsertNextValue(connectorNode->DeviceModifiedEvent);
  vtkObserveMRMLNodeEventsMacro(connectorNode, connectorNodeEvents.GetPointer());
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return;
    }
  vtkUnObserveMRMLNodeMacro(connectorNode);
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::RegisterMessageDevices(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
    {
    return;
    }
  for (unsigned short i = 0; i < this->GetNumberOfDevices(); i ++)
  {
    connectorNode->AddDevice(this->GetDevice(i));
    connectorNode->ConnectEvents();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneEndImport()
{
  // Scene loading/import is finished, so now start the command processing thread
  // of all the active persistent connector nodes

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  for (std::vector< vtkMRMLNode* >::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
      {
      continue;
      }
    if (connector->GetPersistent())
      {
      this->Modified();
      if (connector->GetState() != vtkMRMLIGTLConnectorNode::StateOff)
        {
        connector->Start();
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  //vtkDebugMacro("vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeAdded");

  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
    {
    // TODO Remove this line when the corresponding UI option will be added
    connectorNode->SetRestrictDeviceName(0);

    this->AddMRMLConnectorNodeObserver(connectorNode);
    //this->RegisterMessageDevices(connectorNode);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
    {
    this->RemoveMRMLConnectorNodeObserver(connectorNode);
    }
}

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkSlicerOpenIGTLinkIFLogic::GetConnector(const char* conID)
{
  vtkMRMLNode* node = this->GetMRMLScene()->GetNodeByID(conID);
  if (node)
    {
    vtkMRMLIGTLConnectorNode* conNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    return conNode;
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::CallConnectorTimerHander()
{
  //ConnectorMapType::iterator cmiter;
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

  std::vector<vtkMRMLNode*>::iterator iter;

  //for (cmiter = this->ConnectorMap.begin(); cmiter != this->ConnectorMap.end(); cmiter ++)
  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
      {
      continue;
      }
      connector->PeriodicProcess();
    }
}


//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::SetRestrictDeviceName(int f)
{
  if (f != 0) f = 1; // make sure that f is either 0 or 1.
  this->RestrictDeviceName = f;

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

  std::vector<vtkMRMLNode*>::iterator iter;

  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
    {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector)
      {
      connector->SetRestrictDeviceName(f);
      }
    }
  return this->RestrictDeviceName;
}

//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::RegisterMessageDevice(IGTLDevicePointer devicePtr)
{
  igtlio::Device* Device = static_cast<igtlio::Device*>(devicePtr);
  if (Device == NULL)
    {
    return 0;
    }

  // Search the list and check if the same Device has already been registered.
  int found = 0;

  MessageDeviceListType::iterator iter;
  for (iter = this->Internal->MessageDeviceList.begin();
       iter != this->Internal->MessageDeviceList.end();
       iter ++)
    {
    if (Device->GetDeviceType().c_str() && (strcmp(Device->GetDeviceType().c_str(), (*iter)->GetDeviceType().c_str()) == 0))
      {
      found = 1;
      }
    }
  if (found)
    {
    return 0;
    }

  if (Device->GetDeviceType().c_str())
    // TODO: is this correct? Shouldn't it be "&&"
    {
    this->Internal->MessageDeviceList.push_back(Device);
    //Device->SetOpenIGTLinkIFLogic(this);
    }
  else
    {
    return 0;
    }

  // Add the Device to the existing connectors
  if (this->GetMRMLScene())
    {
    std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

    std::vector<vtkMRMLNode*>::iterator niter;
    for (niter = nodes.begin(); niter != nodes.end(); niter ++)
      {
      vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*niter);
      if (connector)
        {
        connector->AddDevice(Device);
        }
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkIFLogic::UnregisterMessageDevice(IGTLDevicePointer devicePtr)
{
  igtlio::Device* Device = static_cast<igtlio::Device*>(devicePtr);
  if (Device == NULL)
    {
    return 0;
    }

  // Look up the message Device list
  MessageDeviceListType::iterator iter;
  iter = this->Internal->MessageDeviceList.begin();
  while ((*iter) != Device) iter ++;

  if (iter != this->Internal->MessageDeviceList.end())
    // if the Device is on the list
    {
    this->Internal->MessageDeviceList.erase(iter);
    // Remove the Device from the existing connectors
    std::vector<vtkMRMLNode*> nodes;
    if (this->GetMRMLScene())
      {
      this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);

      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
        {
        vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
        if (connector)
          {
          connector->RemoveDevice(Device);
          }
        }
      }
    return 1;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSlicerOpenIGTLinkIFLogic::GetNumberOfDevices()
{
  return this->Internal->MessageDeviceList.size();
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkSlicerOpenIGTLinkIFLogic::GetDevice(unsigned int i)
{

  if (i < this->Internal->MessageDeviceList.size())
    {
    return this->Internal->MessageDeviceList[i];
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkSlicerOpenIGTLinkIFLogic::GetDeviceByMRMLTag(const char* mrmlTag)
{
  //Currently, this function cannot find multiple Devices
  // that use the same mrmlType (e.g. vtkIGTLToMRMLLinearTransform
  // and vtkIGTLToMRMLPosition). A Device that is found first
  // will be returned.

  igtlio::Device* Device = NULL;

  MessageDeviceListType::iterator iter;
  for (iter = this->Internal->MessageDeviceList.begin();
       iter != this->Internal->MessageDeviceList.end();
       iter ++)
    {
    if (strcmp((*iter)->GetDeviceName().c_str(), mrmlTag) == 0)
      {
      Device = *iter;
      break;
      }
    }

  return Device;
}

//---------------------------------------------------------------------------
IGTLDevicePointer vtkSlicerOpenIGTLinkIFLogic::GetDeviceByDeviceType(const char* deviceType)
{
  igtlio::Device* Device = NULL;

  MessageDeviceListType::iterator iter;
  for (iter = this->Internal->MessageDeviceList.begin();
       iter != this->Internal->MessageDeviceList.end();
       iter ++)
    {
      if (strcmp((*iter)->GetDeviceType().c_str(), deviceType) == 0)
      {
        Device = *iter;
        break;
      }
    }

  return Device;
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::ProcessMRMLNodesEvents(vtkObject * caller, unsigned long event, void * callData)
{

  if (caller != NULL)
    {
    vtkSlicerModuleLogic::ProcessMRMLNodesEvents(caller, event, callData);

    vtkMRMLIGTLConnectorNode* cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);
    if (cnode && event == cnode->DeviceModifiedEvent)
      {
      // Check visibility
      int nnodes;

      // Incoming nodes
      nnodes = cnode->GetNumberOfIncomingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
        {
        vtkMRMLNode* inode = cnode->GetIncomingMRMLNode(i);
        if (inode)
          {
          igtlio::Device* Device = static_cast<igtlio::Device*>(GetDeviceByMRMLTag(inode->GetNodeTagName()));
          if (Device)
            {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
              {
              Device->SetVisibility(1);
              }
            else
              {
              Device->SetVisibility(0);
              }
            }
          }
        }

      // Outgoing nodes
      nnodes = cnode->GetNumberOfOutgoingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
        {
        vtkMRMLNode* inode = cnode->GetOutgoingMRMLNode(i);
        if (inode)
          {
          igtlio::Device* Device = static_cast<igtlio::Device*>(GetDeviceByMRMLTag(inode->GetNodeTagName()));
          if (Device)
            {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
              {
              Device->SetVisibility(1);
              }
            else
              {
              Device->SetVisibility(0);
              }
            }
          }
        }
      }
    }

    ////---------------------------------------------------------------------------
    //// Outgoing data
    //// TODO: should check the type of the node here
    //ConnectorListType* list = &MRMLEventConnectorMap[node];
    //ConnectorListType::iterator cliter;
    //for (cliter = list->begin(); cliter != list->end(); cliter ++)
    //  {
    //  vtkMRMLIGTLConnectorNode* connector =
    //    vtkMRMLIGTLConnectorNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(*cliter));
    //  if (connector == NULL)
    //    {
    //    return;
    //    }
    //
    //  MessageDeviceListType::iterator iter;
    //  for (iter = this->Internal->MessageDeviceList.begin();
    //       iter != this->Internal->MessageDeviceList.end();
    //       iter ++)
    //    {
    //    if ((*iter)->GetMRMLName() && strcmp(node->GetNodeTagName(), (*iter)->GetMRMLName()) == 0)
    //      {
    //      // check if the name-type combination is on the list
    //      if (connector->GetDeviceID(node->GetName(), (*iter)->GetIGTLName()) >= 0)
    //        {
    //        int size;
    //        void* igtlMsg;
    //        (*iter)->MRMLToIGTL(event, node, &size, &igtlMsg);
    //        int r = connector->SendData(size, (unsigned char*)igtlMsg);
    //        if (r == 0)
    //          {
    //          // TODO: error handling
    //          //std::cerr << "ERROR: send data." << std::endl;
    //          }
    //        }
    //      }
    //    } // for (iter)
    //  } // for (cliter)
}




//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::ProcCommand(const char* vtkNotUsed(nodeName), int vtkNotUsed(size), unsigned char* vtkNotUsed(data))
{
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list)
{

  list.clear();

  MessageDeviceListType::iterator mcliter;
  for (mcliter = this->Internal->MessageDeviceList.begin();
       mcliter != this->Internal->MessageDeviceList.end();
       mcliter ++)
    {
    if ((*mcliter)->GetDeviceName().c_str())
      {
      std::string className = this->GetMRMLScene()->GetClassNameByTag((*mcliter)->GetDeviceName().c_str());
      std::string deviceTypeName;
      if ((*mcliter)->GetDeviceType().c_str() != NULL)
        {
        deviceTypeName = (*mcliter)->GetDeviceType().c_str();
        }
      else
        {
        deviceTypeName = (*mcliter)->GetDeviceName();
        }
      std::vector<vtkMRMLNode*> nodes;
      this->GetApplicationLogic()->GetMRMLScene()->GetNodesByClass(className.c_str(), nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
        {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name   = (*iter)->GetName();
        nodeInfo.type   = deviceTypeName.c_str();
        nodeInfo.io     = igtlio::Connector::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkIFLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list, const char* mrmlTagName)
{

  list.clear();

  MessageDeviceListType::iterator mcliter;
  for (mcliter = this->Internal->MessageDeviceList.begin();
       mcliter != this->Internal->MessageDeviceList.end();
       mcliter ++)
    {
    if ((*mcliter)->GetDeviceName().c_str() && strcmp(mrmlTagName, (*mcliter)->GetDeviceName().c_str()) == 0)
      {
      const char* className = this->GetMRMLScene()->GetClassNameByTag(mrmlTagName);
      const char* deviceTypeName = (*mcliter)->GetDeviceType().c_str();
      std::vector<vtkMRMLNode*> nodes;
      this->GetMRMLScene()->GetNodesByClass(className, nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
        {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name = (*iter)->GetName();
        nodeInfo.type = deviceTypeName;
        nodeInfo.io   = igtlio::Connector::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
        }
      }
    }
}

////---------------------------------------------------------------------------
//const char* vtkSlicerOpenIGTLinkIFLogic::MRMLTagToIGTLName(const char* mrmlTagName);
//{
//
//}
