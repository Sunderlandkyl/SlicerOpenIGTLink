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
#ifndef __vtkIGTLToMRMLBase_h
#define __vtkIGTLToMRMLBase_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// VTK includes
#include <vtkObject.h>

// std includes
#include <string>
#include <vector>

class igtlioDevice;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLNode;
class vtkMRMLScene;

/// \brief Abstract base class for OpenIGTLink ↔ MRML node converters.
///
/// Subclass this to add support for a new OpenIGTLink device type without
/// modifying vtkMRMLIGTLConnectorNode. Register instances via
/// vtkIGTLIOConverterFactory::GetInstance()->RegisterConverter().
class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLBase : public vtkObject
{
public:
  vtkTypeMacro(vtkIGTLToMRMLBase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return the OpenIGTLink device type string this converter handles (e.g. "IMAGE").
  virtual std::string GetIGTLDeviceType() const = 0;

  /// Return the MRML node tag names this converter can produce/consume
  /// (e.g. {"Volume", "VectorVolume"}).
  virtual std::vector<std::string> GetMRMLNodeTagNames() = 0;

  /// Find an existing compatible MRML node in the scene or create a new one,
  /// populate it with the initial device content, add it to the scene,
  /// and register it with the connector.
  ///
  /// Returns nullptr when the device does not yet carry enough data to construct
  /// a node (e.g. IMAGE device with no image data).
  virtual vtkMRMLNode* CreateNode(vtkMRMLScene* scene,
                                  igtlioDevice* device,
                                  vtkMRMLIGTLConnectorNode* connector) = 0;

  /// Copy data from the incoming device into the already-existing MRML node,
  /// then copy all IGTL metadata entries as "OpenIGTLink.<key>" node attributes.
  virtual void DeviceToMRML(igtlioDevice* device,
                    vtkMRMLNode* node,
                    vtkMRMLIGTLConnectorNode* connector);

  /// Populate the outgoing device from the MRML node.
  /// Returns the VTK event ID that, when fired by the node, should trigger
  /// a push (e.g. vtkMRMLVolumeNode::ImageDataModifiedEvent).
  /// Returns 0 if this converter/device combination is not meant to be pushed.
  virtual unsigned int MRMLToDevice(vtkMRMLNode* node, igtlioDevice* device) = 0;

  /// Set metadata on an outgoing device before sending. Called by PushNode
  /// when the message header version supports metadata (>= IGTL v2).
  /// Default sets MRMLNodeNameKey to the node tag name and GetStatusMetaDataKey()
  /// to the node attribute of the same name, or "OK" when the attribute is absent.
  /// Override to replace or extend.
  virtual void PrepareOutgoingDevice(vtkMRMLNode* node, igtlioDevice* device);

  /// Name of the metadata element (and the MRML node attribute it is read from)
  /// that carries the node's status. Default is "Status"; Transform uses "TransformStatus".
  virtual const char* GetStatusMetaDataKey() const { return "Status"; }

  /// Metadata key used to carry the MRML node type name across the wire.
  static const char* const MRMLNodeNameKey;

protected:
  /// Device-type-specific data transfer. Called by DeviceToMRML before metadata copy.
  virtual void DeviceToMRMLInternal(igtlioDevice* device,
                                    vtkMRMLNode* node,
                                    vtkMRMLIGTLConnectorNode* connector) = 0;

  /// Copy all IGTL metadata entries from device to MRML node attributes
  /// prefixed with "OpenIGTLink.".
  static void CopyMetaDataToMRMLNode(igtlioDevice* device, vtkMRMLNode* node);

  vtkIGTLToMRMLBase();
  ~vtkIGTLToMRMLBase() override;

private:
  vtkIGTLToMRMLBase(const vtkIGTLToMRMLBase&) = delete;
  void operator=(const vtkIGTLToMRMLBase&) = delete;
};

#endif
