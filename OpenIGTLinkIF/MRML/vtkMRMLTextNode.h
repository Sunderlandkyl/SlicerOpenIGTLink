#ifndef __vtkMRMLTextNode_h
#define __vtkMRMLTextNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLStorableNode.h>

// VTK includes
#include <vtkStdString.h>

// STD includes
#include <sstream>

class  VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLTextNode : public vtkMRMLStorableNode
{
public:
  enum
  {
    ENCODING_US_ASCII = 3,
    ENCODING_ISO_8859_1 = 4,
    ENCODING_LATIN1 = ENCODING_ISO_8859_1 // alias
    // see other codes at http://www.iana.org/assignments/character-sets/character-sets.xhtml
  };

  static vtkMRMLTextNode *New();
  vtkTypeMacro(vtkMRMLTextNode, vtkMRMLStorableNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "Text";};

  ///
  /// Set text encoding
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  ///
  /// Set encoding of the text
  /// For character encoding, please refer IANA Character Sets
  /// (http://www.iana.org/assignments/character-sets/character-sets.xhtml)
  /// Default is US-ASCII (ANSI-X3.4-1968; MIBenum = 3).
  vtkSetMacro (Encoding, int);
  vtkGetMacro (Encoding, int);

  enum
  {
    TextModifiedEvent  = 30001,
  };

  ///
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() VTK_OVERRIDE;

protected:
  vtkMRMLTextNode();
  ~vtkMRMLTextNode();
  vtkMRMLTextNode(const vtkMRMLTextNode&);
  void operator=(const vtkMRMLTextNode&);

  char* Text;
  int Encoding;
};

#endif
