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
      
    # Parameter sets
    depthWidget = slicer.qSlicerUltrasoundDepthWidget()
    self.layout.addWidget(depthWidget)
    
    self.layout.addStretch(1)

#
# UltrasoundRemoteControlLogic
#
class UltrasoundRemoteControlLogic(ScriptedLoadableModuleLogic):
  def __init__(self, parent = None):
    ScriptedLoadableModuleLogic.__init__(self, parent)

  def __del__(self):
    # Clean up commands
    pass
