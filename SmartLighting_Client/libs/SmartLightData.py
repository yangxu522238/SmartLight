class SmartLightDataController():
    def __init__(self,isCH):
        self.DataFormat = "yyyy-MM-dd"
        self.TimeFormat = "hh:mm:ss"
        self.DataTimeFormat = "yyyy-MM-dd hh：mm：ss"
        if(isCH):
            self.LocationalData = SmartLightData_CH()
        else:
            self.LocationalData = SmartLightData_EN()

class SmartLightData_EN():
    def __init__(self):
        self.OpenLightBtnTip = "Light is off, you can click button to turn on the light"
        self.CloseLightBtnTip = "Light is on, you can click button to turn off the light"
class SmartLightData_CH():
    def __init__(self):
        self.OpenLightBtnTip = "灯光关闭状态，点击按钮打开灯光"
        self.CloseLightBtnTip = "灯光打开状态，点击按钮关闭灯光"