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
    self.deviceIDLineEdit.setText("VideoDevice")
    plusParametersLayout.addRow("Device ID:", self.deviceIDLineEdit)

    # Ultrasound parameters
    ultrasoundParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    ultrasoundParametersCollapsibleButton.text = "Ultrasound parameters"
    ultrasoundParametersCollapsibleButton.collapsed = False
    self.layout.addWidget(ultrasoundParametersCollapsibleButton)
    ultrasoundParametersLayout = qt.QFormLayout(ultrasoundParametersCollapsibleButton)

    self.depthSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.depthSlider.setParameterName("DepthMm")
    self.depthSlider.setMinimum(10.0)
    self.depthSlider.setMaximum(150.0)
    ultrasoundParametersLayout.addRow("Depth (mm):",  self.depthSlider)
    
    self.gainSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.gainSlider.setParameterName("GainPercent")
    self.gainSlider.setMinimum(0.0)
    self.gainSlider.setMaximum(100.0)
    ultrasoundParametersLayout.addRow("Gain (%):", self.gainSlider)

    self.frequencySlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.frequencySlider.setParameterName("FrequencyMhz")
    self.frequencySlider.setMinimum(2.0)
    self.frequencySlider.setMaximum(5.0)
    ultrasoundParametersLayout.addRow("Frequency (MHz):", self.frequencySlider)

    self.dynamicRangeSlider = slicer.qSlicerUltrasoundDoubleParameterSlider()
    self.dynamicRangeSlider.setParameterName("DynRangeDb")
    self.dynamicRangeSlider.setMinimum(10.0)
    self.dynamicRangeSlider.setMaximum(100.0)
    ultrasoundParametersLayout.addRow("Dynamic Range (dB):", self.dynamicRangeSlider)
    
    self.layout.addStretch(1)
    
    self.parameterWidgets = [
    self.depthSlider,
    self.gainSlider,
    self.frequencySlider,
    self.dynamicRangeSlider,
    ]
    
    self.connectorNodeSelector.connect("nodeActivated(vtkMRMLNode*)", self.onConnectorNodeSelected)
    self.connectorNodeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onConnectorNodeSelected)
    self.deviceIDLineEdit.connect("editingFinished()", self.onDeviceIdChanged())
    
    self.onConnectorNodeSelected(self.connectorNodeSelector.currentNode())
    
  def onReload(self):
    pass
  
  def onConnectorNodeSelected(self, connectorNode):
    for widget in self.parameterWidgets:
      widget.setConnectorNode(connectorNode)

  def onDeviceIdChanged(self):
    deviceID = self.deviceIDLineEdit.text
    for widget in self.parameterWidgets:
      widget.setDeviceID(deviceID)

#
# UltrasoundRemoteControlLogic
#
class UltrasoundRemoteControlLogic(ScriptedLoadableModuleLogic):
  def __init__(self, parent = None):
    ScriptedLoadableModuleLogic.__init__(self, parent)

  def __del__(self):
    # Clean up commands
    pass
