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
#ifndef __vtkIGTLIOConverterFactory_h
#define __vtkIGTLIOConverterFactory_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLBase.h"

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

// std includes
#include <map>
#include <string>
#include <vector>

/// \brief Registry that maps OpenIGTLink device types to MRML converters.
///
/// Use the singleton accessor to register and retrieve converters:
/// \code
///   vtkIGTLIOConverterFactory::GetInstance()->RegisterConverter(
///     vtkSmartPointer<vtkIGTLToMRMLTransform>::New());
/// \endcode
///
/// The default converters for all built-in device types are registered by
/// vtkSlicerOpenIGTLinkIFLogic::RegisterNodes().
class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLIOConverterFactory : public vtkObject
{
public:
  vtkTypeMacro(vtkIGTLIOConverterFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkIGTLIOConverterFactory* New();

  /// Return the process-wide singleton instance.
  static vtkIGTLIOConverterFactory* GetInstance();

  /// Register a converter. If a converter for the same device type was already
  /// registered it is silently replaced.
  void RegisterConverter(vtkIGTLToMRMLBase* converter);

  /// Remove the converter registered for \p deviceType (no-op if none).
  void UnregisterConverter(const std::string& deviceType);

  /// Return the converter for \p deviceType, or nullptr if none is registered.
  vtkIGTLToMRMLBase* GetConverter(const std::string& deviceType);

  /// Return all registered converters.
  std::vector<vtkIGTLToMRMLBase*> GetAllConverters();

  /// Return the MRML node tag names associated with \p deviceType
  /// by querying the registered converter (empty vector if no converter found).
  std::vector<std::string> GetNodeTagsFromDeviceType(const char* deviceType);

  /// Return the device type strings associated with \p nodeTag
  /// by reverse-scanning all registered converters.
  std::vector<std::string> GetDeviceTypesFromNodeTag(const char* nodeTag);

protected:
  vtkIGTLIOConverterFactory();
  ~vtkIGTLIOConverterFactory() override;

private:
  vtkIGTLIOConverterFactory(const vtkIGTLIOConverterFactory&) = delete;
  void operator=(const vtkIGTLIOConverterFactory&) = delete;

  typedef std::map<std::string, vtkSmartPointer<vtkIGTLToMRMLBase>> ConverterMapType;
  ConverterMapType Converters;
};

#endif
