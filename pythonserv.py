from bluepy.btle import Peripheral, Service, Characteristic, UUID
from bluepy.btle import BTLEException
import time
import threading

# Mock barcode reader function
def readBarcode():
    time.sleep(5)  # Simulate delay
    return "1234567890123"  # Example barcode

class MyPeripheral(Peripheral):
    def __init__(self):
        Peripheral.__init__(self)
        
        # Create a service
        service_uuid = UUID("12345678-1234-5678-1234-56789abcdef0")
        service = Service(service_uuid, True)

        # Create a characteristic
        char_uuid = UUID("12345678-1234-5678-1234-56789abcdef1")
        char = Characteristic(
            char_uuid,
            Characteristic.props['READ'] | Characteristic.props['NOTIFY'],
            Characteristic.perms['READ'] | Characteristic.perms['WRITE']
        )
        
        # Add characteristic to service
        service.addCharacteristic(char)
        
        # Add service to peripheral
        self.addService(service)

    def start(self):
        self.setLocalName("MyBLEPeripheral")
        self.startAdvertising()

def advertise():
    p = MyPeripheral()
    try:
        p.start()
        print("Advertising started, press Enter to stop...")

        def notify():
            while True:
                barcode = readBarcode()
                print("Read barcode:", barcode)
                for svc in p.getServices():
                    for char in svc.getCharacteristics():
                        if char.uuid == UUID("12345678-1234-5678-1234-56789abcdef1"):
                            char.write(barcode.encode(), True)

        # Start the barcode reading and notifying thread
        threading.Thread(target=notify, daemon=True).start()

        input()
    finally:
        p.stopAdvertising()
        p.disconnect()
        print("Advertising stopped.")

if __name__ == "__main__":
    advertise()
"""
sudo apt update
sudo apt install python3-pip
sudo pip3 install bluepy
"""
