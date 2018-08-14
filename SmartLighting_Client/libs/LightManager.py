from libs.BleDeviceManager import  BleDevice

class LightController():
    def __init__(self,callbackfunc):
        self.lightIsOpen = False
        self.lightCallBack = callbackfunc

        self.ble = BleDevice()
        print(self.ble.scane())
        self.bleDevice = self.ble.connect_name("Bluegiga CR Demo")  # 修改成设备名字
        self.chars = self.ble.discover_characteristics(self.bleDevice)

        print(self.chars) # uuid

        self.InitLight()

    def ChargeLightControllerState(self):
        if(self.lightIsOpen):
            self.CloseLight()
        else:
            self.OpenLight()

    def InitLight(self):
        print(self.ble.read_characteristics(self.chars[1]['uuid']))#根据值设置lightIsOpen 属性
        self.lightIsOpen = False
        if (self.lightCallBack != None):
            self.lightCallBack(self.lightIsOpen, "")

    def OpenLight(self):
        self.lightIsOpen = True
        self.ble.write_characteristics("hello world", self.chars[0]['uuid'])
        if(self.lightCallBack!=None):
            self.lightCallBack(self.lightIsOpen,"")
    def CloseLight(self):
        self.lightIsOpen = False
        self.ble.write_characteristics("hello world", self.chars[0]['uuid'])
        if (self.lightCallBack != None):
            self.lightCallBack(self.lightIsOpen, "")


