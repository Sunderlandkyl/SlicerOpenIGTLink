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

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

// Qt includes
#include <QDebug>

//
#include "vtkMRMLTextNode.h"

#include <vtkMRMLScene.h>

#include <iostream>
#include <string>
#include <cctype>

// SlicerQt includes
#include "qSlicerTextFileReader.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtksys\SystemTools.hxx>

//-----------------------------------------------------------------------------
class qSlicerTextFileReaderPrivate
{
};

//-----------------------------------------------------------------------------
qSlicerTextFileReader::qSlicerTextFileReader(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerTextFileReaderPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerTextFileReader::~qSlicerTextFileReader()
{
}

//-----------------------------------------------------------------------------
QString qSlicerTextFileReader::description() const
{
  return "Text file";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerTextFileReader::fileType() const
{
  return "Text file";
}

//-----------------------------------------------------------------------------
QStringList qSlicerTextFileReader::extensions() const
{
  QStringList supportedExtensions = QStringList();
  supportedExtensions << "Text file (*.txt)";
  supportedExtensions << "XML document (*.xml)";
  supportedExtensions << "JSON document (*.json)";
  return supportedExtensions;
}

//-----------------------------------------------------------------------------
bool qSlicerTextFileReader::load(const IOProperties& properties)
{
  Q_D(qSlicerTextFileReader);
  if (!properties.contains("fileName"))
  {
    qCritical() << Q_FUNC_INFO << " did not receive fileName property";
    return false;
  }
  QString fileName = properties["fileName"].toString();

  std::string textNodeName = vtksys::SystemTools::GetFilenameWithoutExtension(fileName.toStdString());
  std::string textNodeExtention = vtksys::SystemTools::GetFilenameExtension(fileName.toStdString());
  std::transform(textNodeExtention.begin(), textNodeExtention.end(), textNodeExtention.begin(), ::tolower);

  vtkSmartPointer<vtkMRMLTextNode> textNode = vtkSmartPointer<vtkMRMLTextNode>::New();
  textNode->SetName(this->mrmlScene()->GetUniqueNameByString(textNodeName.c_str()));

  std::ifstream inputFile;
  inputFile.open(fileName.toStdString());

  if (inputFile.fail())
  {
    qCritical() << Q_FUNC_INFO << " could not read file";
    return false;
  }

  std::stringstream ss;
  ss << inputFile.rdbuf();
  std::string inputString = ss.str();
  textNode->SetText(inputString.c_str());

  if (textNodeExtention == "xml" || textNodeExtention == "json")
  {
    textNode->SetForceStorageNode(true);
  }

  this->mrmlScene()->AddNode(textNode);
  return true;
}
