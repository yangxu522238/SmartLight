# -*- coding: utf-8 -*-
from PyQt4.QtCore import QTimer
from PyQt4 import QtGui
from PyQt4.QtCore import QDateTime

class TimeMgr(QtGui.QMainWindow):
    def __init__(self):
        QtGui.QMainWindow.__init__(self)

    def GetSystemDataTime(self):
        return QDateTime.currentDateTime()

    def GetSystemDataTimeStamp(self):
        return self.GetSystemDataTime().toTime_t()

    def StartUpdateDataTime(self,callBackFunc):
        self.updateTimerCallBack=callBackFunc
        self.updateTimer=QTimer(self)
        self.updateTimer.setInterval(1000)
        self.updateTimer.timeout.connect(self.updateTimerCallBackFunc)
        self.updateTimer.start()

    def StopUpdateDataTime(self):
        if(self.updateTimer):
            self.updateTimer.stop()

    def updateTimerCallBackFunc(self):
        if(self.updateTimerCallBack):
            self.updateTimerCallBack(self.GetSystemDataTime())

