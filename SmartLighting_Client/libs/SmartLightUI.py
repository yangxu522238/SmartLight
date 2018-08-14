# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'SmartLightUI.ui'
#
# Created: Tue Jun 19 13:26:21 2018
#      by: PyQt4 UI code generator 4.11
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_SmartLighting(object):
    def setupUi(self, SmartLighting):
        SmartLighting.setObjectName(_fromUtf8("SmartLighting"))
        SmartLighting.resize(800, 600)
        self.centralwidget = QtGui.QWidget(SmartLighting)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.Bg = QtGui.QLabel(self.centralwidget)
        self.Bg.setEnabled(False)
        self.Bg.setGeometry(QtCore.QRect(0, 0, 800, 600))
        self.Bg.setText(_fromUtf8(""))
        self.Bg.setObjectName(_fromUtf8("Bg"))
        self.SysDate = QtGui.QLabel(self.centralwidget)
        self.SysDate.setGeometry(QtCore.QRect(20, 20, 200, 24))
        font = QtGui.QFont()
        font.setPointSize(14)
        self.SysDate.setFont(font)
        self.SysDate.setObjectName(_fromUtf8("SysDate"))
        self.SysTime = QtGui.QLabel(self.centralwidget)
        self.SysTime.setGeometry(QtCore.QRect(580, 20, 200, 24))
        font = QtGui.QFont()
        font.setPointSize(14)
        self.SysTime.setFont(font)
        self.SysTime.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.SysTime.setObjectName(_fromUtf8("SysTime"))
        self.ControlLightBtn = QtGui.QPushButton(self.centralwidget)
        self.ControlLightBtn.setGeometry(QtCore.QRect(350, 250, 100, 100))
        self.ControlLightBtn.setObjectName(_fromUtf8("ControlLightBtn"))
        self.ControlBtnTip = QtGui.QLabel(self.centralwidget)
        self.ControlBtnTip.setGeometry(QtCore.QRect(0, 210, 800, 24))
        self.ControlBtnTip.setAlignment(QtCore.Qt.AlignCenter)
        self.ControlBtnTip.setObjectName(_fromUtf8("ControlBtnTip"))
        SmartLighting.setCentralWidget(self.centralwidget)

        self.retranslateUi(SmartLighting)
        QtCore.QObject.connect(self.ControlLightBtn, QtCore.SIGNAL(_fromUtf8("clicked()")), SmartLighting.OnLightBtnClick)
        QtCore.QMetaObject.connectSlotsByName(SmartLighting)

    def retranslateUi(self, SmartLighting):
        SmartLighting.setWindowTitle(_translate("SmartLighting", "SmartLight", None))
        self.SysDate.setText(_translate("SmartLighting", "Date", None))
        self.SysTime.setText(_translate("SmartLighting", "Time", None))
        self.ControlLightBtn.setText(_translate("SmartLighting", "Close", None))
        self.ControlBtnTip.setText(_translate("SmartLighting", "Control Light", None))

