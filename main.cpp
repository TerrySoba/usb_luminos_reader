#include "tclap/CmdLine.h"
#include "hidapi.h"

#include <iostream>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <codecvt>
#include <exception>
#include <memory>
#include <stdexcept>
#include <typeinfo>

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

int hexStringToInt(const std::string& str)
{
    std::stringstream ss;
    ss << str;
    ss << std::hex;
    int value;
    ss >> value;

    return value;
}

struct HidDevice
{
    std::string devicePath;
    uint16_t vid;
    uint16_t pid;
    std::string serialNumber;
};

struct RequestedDevice
{
    uint16_t vid;
    uint16_t pid;
    std::string serialNumber;
};

/**
 * Outputs a list off all available USB HID devices.
 */
void outputDeviceList()
{
    // Enumerate and print the HID devices on the system
    struct hid_device_info *devs, *cur_dev;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;

    while (cur_dev) {
        printf("Device Found\n  vid: %04hx pid: %04hx\n  path: %s\n  serial_number: %ls",
            cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
        printf("\n");
        printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
        printf("  Product:      %ls\n", cur_dev->product_string);
        printf("\n");
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
}

/**
 * returns the path to the found device, or an empty string if device was not found.
 */
std::string getRequestedDevice(RequestedDevice requestedDevice)
{
    // Enumerate and print the HID devices on the system
    struct hid_device_info *devs, *cur_dev;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    std::vector<HidDevice> devices;
    while (cur_dev) {
        HidDevice dev;
        if (cur_dev->path)
            dev.devicePath = cur_dev->path;
        dev.vid = cur_dev->vendor_id;
        dev.pid = cur_dev->product_id;
        if (cur_dev->serial_number)
            dev.serialNumber = ws2s(cur_dev->serial_number);
        devices.push_back(dev);
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    // search for requested device in list
    auto it = std::find_if(devices.cbegin(), devices.cend(), [requestedDevice](const HidDevice& dev){
        if (dev.pid == requestedDevice.pid && dev.vid == requestedDevice.vid)
        {
            if (requestedDevice.serialNumber.empty())
            {
                return true;
            }
            else if (requestedDevice.serialNumber == dev.serialNumber)
            {

                return true;
            }
        }
        return false;
    });

    if (it != devices.cend())
    {
        return it->devicePath;
    }
    else
    {
        return "";
    }
}

RequestedDevice parseCommandline(int argc, char* argv[])
{
    TCLAP::CmdLine cmd("Tool to read Cleware USB-Luminos Data", ' ', "0.1");
    TCLAP::ValueArg<std::string> vendorId(
        "v",
        "vid",
        "Vendor ID of device in hex",
        false, // required?
        "0d50", // default value
        "string");
    cmd.add( vendorId );

    TCLAP::ValueArg<std::string> productId(
        "p",
        "pid",
        "Product ID of device in hex",
        false, // required?
        "0050", // default value
        "string");
    cmd.add( productId );

    TCLAP::ValueArg<std::string> serialNumber(
        "s",
        "serial",
        "Serial number of device",
        false, // required?
        "", // default value
        "string");
    cmd.add( serialNumber );


    TCLAP::SwitchArg listDevices(
        "l",
        "list",
        "List all available USB devices",
        false);
    cmd.add( listDevices );

    // Parse the args.
    cmd.parse( argc, argv );

    if (listDevices.getValue())
    {
        outputDeviceList();
        exit(0);
    }

    RequestedDevice args;
    args.vid = hexStringToInt(vendorId.getValue());
    args.pid = hexStringToInt(productId.getValue());
    args.serialNumber = serialNumber.getValue();

    return args;
}

/**
 * Decodes the data returned by the Cleware USB-Luminos sensor.
 *
 * 0x0000 is very dark, 0xffff would be extremely bright.
 *
 * This has been reverse engineered, so no guarantees that it is
 * 100% correct.
 *
 * @param data The data returned by the sensor.
 * @param len The length of the data in bytes.
 * @return The decoded sensor value.
 */
uint16_t decodeLuminosData(void* data, size_t len)
{
    if (len < 4) return 0;
    auto ptr = reinterpret_cast<uint8_t*>(data);
    return ptr[2] << 8 | ptr[3];
}

/**
 * Opens the USB device and reads the sensor value from it.
 *
 * @param path The path of the USB device.
 * @return The sensor value.
 */
uint16_t getSensorValue(const std::string& path)
{
    auto device = std::shared_ptr<hid_device>(
                hid_open_path(path.c_str()),
                [](hid_device* ptr){ if (ptr) hid_close(ptr); });
    if (!device) throw std::runtime_error("Could not open usb device.");

    unsigned char buf[16];
    int res = hid_read(device.get(), buf, sizeof(buf));
    uint16_t sensorValue = decodeLuminosData(buf, res);

    return sensorValue;
}

/**
 * This tool is used to read the value from the Cleware USB-Luminos sensor
 * available from http://www.cleware-shop.de/
 */
int main(int argc, char* argv[])
{
    try
    {
        auto requestedDevice = parseCommandline(argc, argv);
        auto path = getRequestedDevice(requestedDevice);
        if (path.empty())
        {
            std::cerr << "Device not found. Try -l to list all available devices." << std::endl;
            return 1;
        }
        else
        {
            auto value = getSensorValue(path);
            std::cout << value << std::endl;
            hid_exit();
            return 0;
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception " << typeid(ex).name() << ". what() == \"" << ex.what() << "\"" << std::endl;
    }
    catch (...)
    {
        std::cerr << "Caught unknown exception." << std::endl;
    }
    return 1;
}
