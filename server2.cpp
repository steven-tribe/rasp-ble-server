#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <glib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
// g++ ble_peripheral.cpp -o ble_peripheral -lbluetooth -pthread
// sudo apt update
// sudo apt install bluez libbluetooth-dev libglib2.0-dev


static const char *LE_ADVERTISE = "LE_ADVERTISE";

class MockBarcodeReader {
public:
    std::string readBarcode() {
        // Simulate reading a barcode
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Simulate delay
        return "1234567890123"; // Example barcode
    }
};

void advertise(int adapter_id, MockBarcodeReader &barcodeReader) {
    int device;
    if ((device = hci_open_dev(adapter_id)) < 0) {
        std::cerr << "Unable to open device" << std::endl;
        return;
    }

    uint8_t adv_data[] = {
        0x02, 0x01, 0x06,  // flags: LE General Discoverable Mode, BR/EDR Not Supported
        0x11, 0x07, 0xD6, 0x22, 0x67, 0xE7, 0xB7, 0xA5, 0x97, 0xD3, 0x4F, 0x5B, 0xD5, 0x9B, 0x2B, 0x33, 0x8F, 0xD5, 0x66  // 128-bit UUID
    };

    if (hci_le_set_advertising_data(device, sizeof(adv_data), adv_data, 1000) < 0) {
        std::cerr << "Failed to set advertising data" << std::endl;
        return;
    }
    
    if (hci_le_set_advertise_enable(device, 1, 1000) < 0) {
        std::cerr << "Failed to enable advertising" << std::endl;
        return;
    }

    std::cout << "Advertising started, press Enter to stop..." << std::endl;

    std::thread([&]() {
        while (true) {
            std::string barcode = barcodeReader.readBarcode();
            std::cout << "Read barcode: " << barcode << std::endl;
            // Add code to send the barcode data as a BLE notification here
        }
    }).detach();

    std::cin.ignore();

    if (hci_le_set_advertise_enable(device, 0, 1000) < 0) {
        std::cerr << "Failed to disable advertising" << std::endl;
        return;
    }

    hci_close_dev(device);
}

int main() {
    int adapter_id = hci_get_route(NULL);
    if (adapter_id < 0) {
        std::cerr << "No Bluetooth Adapter Found!" << std::endl;
        return -1;
    }

    MockBarcodeReader barcodeReader;
    advertise(adapter_id, barcodeReader);
    return 0;
}
