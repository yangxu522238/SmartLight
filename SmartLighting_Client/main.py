import sys,os
import libs.LightManager
import libs.SmartLightData
import libs.TimeManager

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import QTimer
from libs.SmartLightUI import Ui_SmartLighting


class MyApp(QtGui.QMainWindow, Ui_SmartLighting):
    def __init__(self):
        QtGui.QMainWindow.__init__(self)
        Ui_SmartLighting.__init__(self)
        self.setupUi(self)
        self._smarltLightData = libs.SmartLightData.SmartLightDataController(True)
        self._lightController = libs.LightManager.LightController(self.OnLightChanged)
        self._timeMgr = libs.TimeManager.TimeMgr()
        self.UpdateDataTimeLabel(self._timeMgr.GetSystemDataTime())
        self._timeMgr.StartUpdateDataTime(self.UpdateDataTimeLabel)
        self.SysTip.setText("")

    def OnLightBtnClick(self):
        self._lightController.ChargeLightControllerState()

    def OnLightChanged(self,lightIsOpen,tipCode):
        if(tipCode!=""):
            self.SysTip.setText(self._smarltLightData.LocationalData.ControlLightError + tipCode)
        else:
            if(lightIsOpen):
                self.ControlLightBtn.setText("Close")
                self.ControlBtnTip.setText(self._smarltLightData.LocationalData.CloseLightBtnTip)
            else:
                self.ControlLightBtn.setText("Open")
                self.ControlBtnTip.setText(self._smarltLightData.LocationalData.OpenLightBtnTip)

    def UpdateDataTimeLabel(self, dataTime):
        self.SysDate.setText(dataTime.toString(self._smarltLightData.DataFormat))
        self.SysTime.setText(dataTime.toString(self._smarltLightData.TimeFormat))

if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)
    window = MyApp()
    window.show()
    sys.exit(app.exec_())

