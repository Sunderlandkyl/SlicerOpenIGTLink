import os.path, datetime
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# UltrasoundRemoteControl
#

class UltrasoundRemoteControl(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Ultrasound Remote Control"
    self.parent.categories = ["IGT"]
    self.parent.dependencies = ["OpenIGTLinkRemote"]
    self.parent.contributors = ["Kyle Sunderland (PerkLab, Queen's University), Tamas Ungi (PerkLab, Queen's University), Andras Lasso (PerkLab, Queen's University)"]
    self.parent.helpText = """TODO"""
    self.parent.acknowledgementText = """This work was funded by Cancer Care Ontario Applied Cancer Research Unit (ACRU) and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO) grants."""

#
# qUltrasoundRemoteControlWidget
#
class UltrasoundRemoteControlWidget(ScriptedLoadableModuleWidget):

  def __init__(self, parent = None):
    ScriptedLoadableModuleWidget.__init__(self, parent)

  def setup(self):
    # Instantiate and connect widgets
    ScriptedLoadableModuleWidget.setup(self)

    # Module requires openigtlinkremote
    try:
      slicer.modules.openigtlinkremote
    except:
      self.errorLabel = qt.QLabel("Could not find OpenIGTLink Remote module")
      self.layout.addWidget(self.errorLabel)
      return

    # Plus remote launcher

    #self.plusServerLauncher = 

    # Plus parameters
    plusParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    plusParametersCollapsibleButton.text = "Plus parameters"
    plusParametersCollapsibleButton.collapsed = False
    self.layout.addWidget(plusParametersCollapsibleButton)
    plusParametersLayout = qt.QFormLayout(plusParametersCollapsibleButton)

    self.connectorNodeSelector = slicer.qMRMLNodeComboBox()
    self.connectorNodeSelector.nodeTypes = ( ("vtkMRMLIGTLConnectorNode"), "" )
    self.connectorNodeSelector.setMRMLScene(slicer.mrmlScene)
    plusParametersLayout.addRow("Connector node:", self.connectorNodeSelector)
    
    self.deviceIDLineEdit = qt.QLineEdit()
    plusParametersLayout.addRow("Device ID:", self.deviceIDLineEdit)

    # Ultrasound parameters
    ultrasoundParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    ultrasoundParametersCollapsibleButton.text = "Ultrasound parameters"
    ultrasoundParametersCollapsibleButton.collapsed = False
    self.layout.addWidget(ultrasoundParametersCollapsibleButton)
    ultrasoundParametersLayout = qt.QFormLayout(ultrasoundParametersCollapsibleButton)
    
    self.depthWidget = slicer.qSlicerUltrasoundDepthWidget()
    ultrasoundParametersLayout.addWidget(self.depthWidget)
    
    self.gainWidget = slicer.qSlicerUltrasoundGainWidget()
    ultrasoundParametersLayout.addWidget(self.gainWidget)

    self.frequencyWidget = slicer.qSlicerUltrasoundFrequencyWidget()
    ultrasoundParametersLayout.addWidget(self.frequencyWidget)

    self.dunamicRangeWidget = slicer.qSlicerUltrasoundDynamicRangeWidget()
    ultrasoundParametersLayout.addWidget(self.dunamicRangeWidget)
    
    self.layout.addStretch(1)
    
    self.parameterWidgets = [
    self.depthWidget,
    self.gainWidget,
    self.frequencyWidget,
    self.dunamicRangeWidget,
    ]
    
    self.connectorNodeSelector.connect("nodeActivated(vtkMRMLNode*)", self.onConnectorNodeSelected)

  def onReload(self):
    pass
  
  def onConnectorNodeSelected(self, connectorNode):
    for widget in self.parameterWidgets:
      widget.setConnectorNode(connectorNode)

#
# UltrasoundRemoteControlLogic
#
class UltrasoundRemoteControlLogic(ScriptedLoadableModuleLogic):
  def __init__(self, parent = None):
    ScriptedLoadableModuleLogic.__init__(self, parent)

  def __del__(self):
    # Clean up commands
    pass
