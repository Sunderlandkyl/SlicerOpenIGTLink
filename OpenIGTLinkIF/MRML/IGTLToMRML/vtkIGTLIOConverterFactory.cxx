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

#include "vtkIGTLIOConverterFactory.h"

// VTK includes
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLIOConverterFactory);

//----------------------------------------------------------------------------
vtkIGTLIOConverterFactory* vtkIGTLIOConverterFactory::GetInstance()
{
  static vtkSmartPointer<vtkIGTLIOConverterFactory> instance =
    vtkSmartPointer<vtkIGTLIOConverterFactory>::New();
  return instance;
}

//----------------------------------------------------------------------------
vtkIGTLIOConverterFactory::vtkIGTLIOConverterFactory()
{
}

//----------------------------------------------------------------------------
vtkIGTLIOConverterFactory::~vtkIGTLIOConverterFactory()
{
}

//----------------------------------------------------------------------------
void vtkIGTLIOConverterFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Registered converters: " << this->Converters.size() << "\n";
  for (const auto& kv : this->Converters)
  {
    os << indent << indent << kv.first << "\n";
  }
}

//----------------------------------------------------------------------------
void vtkIGTLIOConverterFactory::RegisterConverter(vtkIGTLToMRMLBase* converter)
{
  if (!converter)
  {
    vtkErrorMacro("RegisterConverter: converter is nullptr");
    return;
  }
  this->Converters[converter->GetIGTLDeviceType()] = converter;
}

//----------------------------------------------------------------------------
void vtkIGTLIOConverterFactory::UnregisterConverter(const std::string& deviceType)
{
  this->Converters.erase(deviceType);
}

//----------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkIGTLIOConverterFactory::GetConverter(const std::string& deviceType)
{
  auto it = this->Converters.find(deviceType);
  if (it == this->Converters.end())
  {
    return nullptr;
  }
  return it->second.GetPointer();
}

//----------------------------------------------------------------------------
std::vector<vtkIGTLToMRMLBase*> vtkIGTLIOConverterFactory::GetAllConverters()
{
  std::vector<vtkIGTLToMRMLBase*> result;
  result.reserve(this->Converters.size());
  for (auto& kv : this->Converters)
  {
    result.push_back(kv.second.GetPointer());
  }
  return result;
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLIOConverterFactory::GetNodeTagsFromDeviceType(const char* deviceType)
{
  if (!deviceType)
  {
    return {};
  }
  vtkIGTLToMRMLBase* converter = this->GetConverter(std::string(deviceType));
  if (!converter)
  {
    return {};
  }
  return converter->GetMRMLNodeTagNames();
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkIGTLIOConverterFactory::GetDeviceTypesFromNodeTag(const char* nodeTag)
{
  if (!nodeTag)
  {
    return {};
  }
  std::vector<std::string> result;
  for (auto& kv : this->Converters)
  {
    for (const std::string& tag : kv.second->GetMRMLNodeTagNames())
    {
      if (tag == nodeTag)
      {
        result.push_back(kv.first);
        break;
      }
    }
  }
  return result;
}
