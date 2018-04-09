#include "vtkSlicerOpenIGTLinkCommand.h"

#include <sstream>

#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerOpenIGTLinkCommand);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkSlicerOpenIGTLinkCommand::vtkSlicerOpenIGTLinkCommand()
: CommandName("")
, ID("")
, DeviceID("")
, CommandXML(NULL)
, CommandTimeoutSec(10)
, ExportedCommandText("")
, ResponseXML(NULL)
, ResponseTextInternal("")
, Status(CommandUnknown)
, Direction(CommandOut)
, Blocking(true)
, CommandVersion(IGTL_HEADER_VERSION_1)
{
  this->CommandXML = vtkXMLDataElement::New();
  this->CommandXML->SetName("Command");
}

//----------------------------------------------------------------------------
vtkSlicerOpenIGTLinkCommand::~vtkSlicerOpenIGTLinkCommand()
{
  if (this->CommandXML != NULL)
    {
    this->CommandXML->Delete();
    this->CommandXML=NULL;
    }
  if (this->ResponseXML != NULL)
    {
    this->ResponseXML->Delete();
    this->ResponseXML=NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ID: " << ( (!this->GetID().empty()) ? this->GetID() : "None" ) << "\n";
  os << "Status: " << vtkSlicerOpenIGTLinkCommand::StatusToString(this->GetStatus()) << "\n";
  os << "CommandText: " << ( (!this->GetCommandText().empty()) ? this->GetCommandText() : "None" ) << "\n";
  os << "CommandXML: ";
  if (this->CommandXML)
    {
    this->CommandXML->PrintXML(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << "\n";
    }
  os << "ResponseText: " << ( (!this->GetResponseText().empty()) ? this->GetResponseText() : "None" ) << "\n";
  os << "ResponseXML: ";
  if (this->ResponseXML)
    {
    this->ResponseXML->PrintXML(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << "\n";
    }
  os << "DeviceId: " << this->GetDeviceID() << "\n";
  os << "CommandId: " << this->GetQueryID() << "\n";
  os << "Version: " << this->GetCommandVersion() << "\n";
}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetCommandAttribute(const std::string& attName)
{
  return std::string(this->CommandXML->GetAttribute(attName.c_str()));
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetCommandAttribute(const std::string& attName, const std::string& attValue)
{
  this->CommandXML->SetAttribute(attName.c_str(), attValue.c_str());
}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetCommandText()
{
  std::ostringstream os;
  this->CommandXML->PrintXML(os, vtkIndent(0));
  this->SetExportedCommandText(os.str().c_str());
  return this->ExportedCommandText;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::SetCommandText(const std::string& text)
{
  if (text.empty())
    {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: empty input");
    return false;
    }

  vtkSmartPointer<vtkXMLDataElement> parsedElem = vtkSmartPointer<vtkXMLDataElement>::Take(vtkXMLUtilities::ReadElementFromString(text.c_str()));
  if (parsedElem == NULL)
  {
    vtkErrorMacro("Failed to set OpenIGTLink command from text: not an XML string: "<<text);
    return false;
  }

  this->CommandXML->DeepCopy(parsedElem);
  return true;
}

//----------------------------------------------------------------------------
int vtkSlicerOpenIGTLinkCommand::GetNumberOfResponses()
{
  int numberOfNestedElements = this->ResponseXML->GetNumberOfNestedElements();
  return numberOfNestedElements;
}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetResponseMessage(int responseID/*=0*/)
{
  if (this->ResponseXML == nullptr)
    {
    return "";
    }

  if (responseID == 0)
    {
    // We might expect the response message to be stored in XML as:
    // <Command Message="MESSAGE_TEXT"></Command>
    std::string responseAttribute = this->GetResponseAttribute("Message");
    if (!responseAttribute.empty())
      {
      return responseAttribute;
      }
    }

  // If responseID is > 0 or if above is not true, look instead for:
  // <Command><Response Success="true/false" Message="MESSAGE_TEXT"></Response></Command>
  int numberOfNestedElements = this->ResponseXML->GetNumberOfNestedElements();
  if (responseID >= numberOfNestedElements)
    {
    vtkErrorMacro("Could not find requested command response: responseID is out of range (Number of responses is " << numberOfNestedElements << ")");
    return "";
    }

  vtkXMLDataElement* nestedElement = this->ResponseXML->GetNestedElement(responseID);
  if (!nestedElement)
    {
    vtkErrorMacro("Could not find requested command response");
    return "";
    }

  const char* message = nestedElement->GetAttribute("Message");
  if (message != nullptr)
    {
    return nestedElement->GetCharacterData();
    }

  vtkErrorMacro("Could not find response message");
  return "";
}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetResponseAttribute(const std::string& attName)
{
  if (this->ResponseXML==NULL)
    {
    return NULL;
    }
  return std::string(this->ResponseXML->GetAttribute(attName.c_str()));
}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::GetResponseText()
{
  return this->ResponseTextInternal;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetResponseText(const std::string& text)
{
  this->SetResponseTextInternal(text);

  if (this->ResponseXML)
    {
    this->ResponseXML->Delete();
    this->ResponseXML=NULL;
    }

  if (text.empty())
  {
    this->SetStatus(CommandFail);
    return;
  }

  this->ResponseXML = vtkXMLUtilities::ReadElementFromString(text.c_str());
  if (this->ResponseXML == NULL)
  {
    // The response is not XML
    vtkWarningMacro("OpenIGTLink command response is not XML: "<<text);
    this->SetStatus(CommandFail);
    return;
  }

}

//----------------------------------------------------------------------------
std::string vtkSlicerOpenIGTLinkCommand::StatusToString(int status)
{
  switch (status)
  {
  case CommandUnknown: return "Unknown";
  case CommandWaiting: return "Waiting";
  case CommandSuccess: return "Success";
  case CommandFail: return "Fail";
  case CommandExpired: return "Expired";
  case CommandCancelled: return "Cancelled";
  default:
    return "Invalid";
  }
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsInProgress()
{
  return this->Status == CommandWaiting;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsCompleted()
{
  return
    this->Status == CommandSuccess ||
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsSucceeded()
{
  return this->Status == CommandSuccess;
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::IsFailed()
{
  return
    this->Status == CommandFail ||
    this->Status == CommandExpired ||
    this->Status == CommandCancelled;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetMetaDataElement(const std::string& key, IANA_ENCODING_TYPE encoding, const std::string& value)
{
  this->MetaData[key] = std::pair<IANA_ENCODING_TYPE, std::string>(encoding, value);
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetMetaDataElement(const std::string& key, const std::string& value)
{
  this->MetaData[key] = std::pair<IANA_ENCODING_TYPE, std::string>(IANA_TYPE_US_ASCII, value);
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::GetMetaDataElement(const std::string& key, IANA_ENCODING_TYPE& outEncoding, std::string& outValue)
{
  if (this->MetaData.find(key) == this->MetaData.end())
  {
    return false;
  }

  outEncoding = this->MetaData[key].first;
  outValue = this->MetaData[key].second;

  return true;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::ClearMetaData()
{
  this->MetaData.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetReponseMetaDataElement(const std::string& key, IANA_ENCODING_TYPE encoding, const std::string& value)
{
  this->SetCommandVersion(IGTL_HEADER_VERSION_2);
  this->ResponseMetaData[key] = std::pair<IANA_ENCODING_TYPE, std::string>(encoding, value);
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::SetReponseMetaDataElement(const std::string& key, const std::string& value)
{
  this->SetCommandVersion(IGTL_HEADER_VERSION_2);
  this->ResponseMetaData[key] = std::pair<IANA_ENCODING_TYPE, std::string>(IANA_TYPE_US_ASCII, value);
}

//----------------------------------------------------------------------------
bool vtkSlicerOpenIGTLinkCommand::GetReponseMetaDataElement(const std::string& key, IANA_ENCODING_TYPE& outEncoding, std::string& outValue)
{
  if (this->ResponseMetaData.find(key) == this->ResponseMetaData.end())
  {
    return false;
  }

  outEncoding = this->ResponseMetaData[key].first;
  outValue = this->ResponseMetaData[key].second;

  return true;
}

//----------------------------------------------------------------------------
igtl::MessageBase::MetaDataMap vtkSlicerOpenIGTLinkCommand::GetResponseMetaData() const
{
  return this->ResponseMetaData;
}

//----------------------------------------------------------------------------
void vtkSlicerOpenIGTLinkCommand::ClearResponseMetaData()
{
  this->ResponseMetaData.clear();
}
