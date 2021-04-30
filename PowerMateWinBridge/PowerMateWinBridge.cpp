// PowerMateWinBridge.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/**
* PowerMate Bluetooth Windows Bridge by github.com/LiveSparks
* Most of the code is stolen from https://github.com/s7726/PowerMate
* Each action of the PowerMate is mapped to one of the unused Function Keys
*/

#include "pch.h"

#define F13 124
#define F14 125
#define F15 126
#define F16 127
#define F17 128
#define F18 129
#define F19 130
#define F20 131
#define F21 132
#define F22 133
#define F23 134
#define F24 135

//Global Variables
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Web::Syndication;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;


enum class STATE_MACHINE
{
    INIT,
    DEVICE_SEARCHING,
    DEVICE_FOUND,
    DEVICE_SELECTING,
    DEVICE_SELECTED,
    SERVICE_SEARCHING,
    SERVICE_FOUND,
    CHARACTERISTICS_SEARCHING,
    CHARACTERISTICS_FOUND
};
STATE_MACHINE myStatus{ STATE_MACHINE::INIT };

enum class KEY_ACTIONS{
    RELEASE,
    PRESS,
    CLICK
};

enum class POWERMATE_ACTIONS : uint8_t
{
    PRESS           = 0x65,
    LONG_RELEASE    = 0x66, // This seems to be less reliably sent
    LEFT            = 0x67,
    RIGHT           = 0x68,
    PRESSED_LEFT    = 0x69,
    PRESSED_RIGHT   = 0x70,
    HOLD_1          = 0x71,
    HOLD_2          = 0x72,
    HOLD_3          = 0x74,
    HOLD_4          = 0x75,
    HOLD_5          = 0x76,
    HOLD_6          = 0x77
};

Windows::Devices::Enumeration::DeviceWatcher deviceWatcher{ nullptr };
Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };

event_token deviceWatcherAddedToken;
event_token deviceWatcherUpdatedToken;
event_token deviceWatcherEnumerationCompletedToken;

std::wstring powerMateDeviceId;

std::map<GattDeviceService, std::set<GattCharacteristic>> powerMateServices;

std::wstring uuid_input{ L"{9cf53570-ddd9-47f3-ba63-09acefc60415}" };
std::wstring uuid_led{ L"847d189e-86ee-4bd2-966f-800832b1259d" };

/*
* @breif    Emulate keyboard key presses
* @param    key:    Key to emulate
* @param    action: Action to take for the key (PRESS, RELEASE, CLICK)
*/
void sendKey(uint8_t key, KEY_ACTIONS action) {

    //if click action, press and release the key
    if (action == KEY_ACTIONS::CLICK) {
        sendKey(key, KEY_ACTIONS::PRESS);
        sendKey(key, KEY_ACTIONS::RELEASE);
    }

    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk = key;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    if (action == KEY_ACTIONS::PRESS)
    {
        ip.ki.dwFlags = 0;  //0 means press the key
    }
    SendInput(1, &ip, sizeof(INPUT));
}

/*
* @brief    Translate PowerMate inputs into keyboard actions
* @param    value: PowerMate Input value
*/
void actOnPowerMate(const uint8_t value) {
    uint8_t key;
    KEY_ACTIONS action = KEY_ACTIONS::CLICK;    //default action is to click

    bool noKey = false;

    switch (static_cast<POWERMATE_ACTIONS>(value))
    {
    case(POWERMATE_ACTIONS::PRESS):
        std::wcout << "INPUT DETECTED: PRESS\n";
        key = F13;
        break;
    case(POWERMATE_ACTIONS::LEFT):
        std::wcout << "INPUT DETECTED: COUTER_CLOCKWISE\n";
        key = F14;
        break;
    case(POWERMATE_ACTIONS::RIGHT):
        std::wcout << "INPUT DETECTED: CLOCKWISE\n";
        key = F15;
        break;
    case(POWERMATE_ACTIONS::PRESSED_LEFT):
        std::wcout << "INPUT DETECTED: PRESSED_COUTER_CLOCKWISE\n";
        key = F16;
        break;
    case(POWERMATE_ACTIONS::PRESSED_RIGHT):
        std::wcout << "INPUT DETECTED: PRESSED_CLOCKWISE\n";
        key = F17;
        break;
    case(POWERMATE_ACTIONS::HOLD_1):
        std::wcout << "INPUT DETECTED: LONGPRESS_1\n";
        key = F18;
        break;
    case(POWERMATE_ACTIONS::HOLD_2):
        std::wcout << "INPUT DETECTED: LONGPRESS_2\n";
        noKey = true;
        break;
    case(POWERMATE_ACTIONS::HOLD_3):
        std::wcout << "INPUT DETECTED: LONGPRESS_3\n";
        noKey = true;
        break;
    case(POWERMATE_ACTIONS::HOLD_4):
        std::wcout << "INPUT DETECTED: LONGPRESS_4\n";
        noKey = true;
        break;
    case(POWERMATE_ACTIONS::HOLD_5):
        std::wcout << "INPUT DETECTED: LONGPRESS_5\n";
        noKey = true;
        break;
    case(POWERMATE_ACTIONS::HOLD_6):
        std::wcout << "INPUT DETECTED: LONGPRESS_6\n";
        noKey = true;
        break;
    case(POWERMATE_ACTIONS::LONG_RELEASE):
        std::wcout << "INPUT DETECTED: RELEASE\n";
        noKey = true;
        break;
    default:
        noKey = true;
        break;
    }

    if(!noKey)
        sendKey(key, action);
}

/*
* @brief    BLE Watcher Device Added Callback. Called when new device is found.
*           If the PowerMate is detected, it's ID is stored in a global variable.
*/
void deviceAdded(DeviceWatcher sender, DeviceInformation deviceInfo)
{
    //Name of the device we are looking for
    std::wstring desiredDevice{ L"PowerMate Bluetooth" };

    //Name of the device that was found
    std::wstring deviceName{ deviceInfo.Name().c_str() };

    //ID of the device that was found
    std::wstring deviceId{ deviceInfo.Id().c_str() };

    //Print the ID and name of the device found
    //std::wcout << "Found\n" << deviceId << ": " << deviceName << "\n";
    
    //If PowerMate is Found, store it's ID
    if (deviceName.compare(desiredDevice) == 0)
    {
        std::wcout << "Found PowerMate Bluetooth:\t" << deviceId << "\n";
        powerMateDeviceId = deviceId;
        myStatus = STATE_MACHINE::DEVICE_FOUND;
    }
}

/*
* @brief    BLE Watcher Device Updated Callback. Called when a device is updated.
*/
void deviceUpdated(DeviceWatcher sender, DeviceInformationUpdate deviceInfo)
{
//    std::wcout << "Updated\n" << deviceInfo.Id().c_str() << "\n";
}

/*
* @brief    BLE Watcher Enumeration Completed Callback. Called when a scanning pass has finished.
*/
void enumComplete(DeviceWatcher sender, IInspectable args)
{
    std::wcout << "Scanning Pass Completed\n";
}

/*
* @brief    BLE Characteristic Value Updated Callback. Called when a value you are subscribed to is updated.
*/
void Characteristic_ValueChanged(GattCharacteristic const& c, GattValueChangedEventArgs args)
{
    // Get the value that has been changed
    IBuffer value_buffer{ args.CharacteristicValue() };
    uint8_t value{ DataReader::FromBuffer(value_buffer).ReadByte() };

    actOnPowerMate(value);
}

/**
* @brief    Start watching for BLE Devices. Attaches BLE Callbacks
*/
void StartBleDeviceWatcher() {
    // The Properties that we want from each BLE device.
    auto requestedProperties = single_threaded_vector<hstring>({ L"System.Devices.Aep.DeviceAddress", L"System.Devices.Aep.IsConnected", L"System.Devices.Aep.Bluetooth.Le.IsConnectable" });

    // BT_Code: Example showing paired and non-paired in a single query.
    hstring aqsAllBluetoothLEDevices = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";

    //Configure BLE Watcher
    deviceWatcher = Windows::Devices::Enumeration::DeviceInformation::CreateWatcher(
            aqsAllBluetoothLEDevices,
            requestedProperties,
            DeviceInformationKind::AssociationEndpoint);

    //Attach Callbacks
    deviceWatcherAddedToken = deviceWatcher.Added(deviceAdded);
    deviceWatcherUpdatedToken = deviceWatcher.Updated(deviceUpdated);
    deviceWatcherEnumerationCompletedToken = deviceWatcher.EnumerationCompleted(enumComplete);

    //Start BLE Watcher.
    deviceWatcher.Start();
    myStatus = STATE_MACHINE::DEVICE_SEARCHING;
}

/**
* @brief    Stops watching for BLE Devices. Unregister BLE Callbacks
*/
void StopBleDeviceWatcher()
{
    if (deviceWatcher != nullptr)
    {
        // Unregister the event handlers.
        deviceWatcher.Added(deviceWatcherAddedToken);
        deviceWatcher.Updated(deviceWatcherUpdatedToken);
        deviceWatcher.EnumerationCompleted(deviceWatcherEnumerationCompletedToken);

        // Stop the watcher.
        deviceWatcher.Stop();
        deviceWatcher = nullptr;
    }
}

/*
* @brief    Connect to a BLE Device
* @param    deviceId: BLE ID of the device to which to connect
* @return   fire_and_forget: because the co_await function needs to run asyncronously
*/
fire_and_forget selectDevice(std::wstring deviceId) {
    myStatus = STATE_MACHINE::DEVICE_SELECTING;
    
    //try to connect to the device
    try {
        bluetoothLeDevice = co_await BluetoothLEDevice::FromIdAsync(deviceId);

        //Connection Error
        if (bluetoothLeDevice == nullptr)
        {
            std::cerr << "Failed to connect to device. Trying again...\n";
            myStatus = STATE_MACHINE::DEVICE_FOUND;
            co_return;
        }
        //Successfully Connected
        else {
            std::wcout << "Connected Successfully!\n";
            myStatus = STATE_MACHINE::DEVICE_SELECTED;
        }
    }
    //Potential hardware error
    catch (hresult_error& ex) {
        if (ex.to_abi() == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE))
        {
            std::cerr << "Bluetooth is not ON!\n";  //unless bluetooth was turned off midway, this will never happen.
        }
        else {
            std::cerr << "Unknown Error Occured\n";
        }
    }
}

/*
* @brief    Get the services from the connected BLE Devices
* @return   fire_and_forget: because the co_await function needs to run asyncronously
*/
fire_and_forget getServices()
{
    myStatus = STATE_MACHINE::SERVICE_SEARCHING;

    // Get the Services
    GattDeviceServicesResult result = co_await bluetoothLeDevice.GetGattServicesAsync(BluetoothCacheMode::Uncached);

    if (result.Status() == GattCommunicationStatus::Success)
    {
        IVectorView<GattDeviceService> services = result.Services();

        //print number of services
        std::wcout << L"Found " << services.Size() << L" services" << "\n";
        for (auto&& service : services)
        {
            powerMateServices[service] = {};
        }
        myStatus = STATE_MACHINE::SERVICE_FOUND;
    }
    else
    {
        std::wcerr << L"Error getting Services! Retrying..." << "\n";
        myStatus = STATE_MACHINE::DEVICE_SELECTED;
    }
}

/*
* @brief    Subscribe to the value changes of a Characteristic. Register the callback for it.
* @param    c:  Characteristic to subscribe to
* @return   fire_and_forget: because the co_await function needs to run asyncronously
*/
fire_and_forget subscribeToValueChange(GattCharacteristic c)
{
    //Check the properties of the Characteristic
    GattClientCharacteristicConfigurationDescriptorValue cccdValue = GattClientCharacteristicConfigurationDescriptorValue::None;
    if ((c.CharacteristicProperties() & GattCharacteristicProperties::Indicate) != GattCharacteristicProperties::None)
    {
        cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Indicate;
    }
    else if ((c.CharacteristicProperties() & GattCharacteristicProperties::Notify) != GattCharacteristicProperties::None)
    {
        cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Notify;
    }
    else
    {
        std::cerr << L"Couldn't set Characteristic Configuration Descriptor" << "\n";
        myStatus = STATE_MACHINE::INIT;
        co_return;
    }

    try
    {
        // Tell the device that we want to subscribe to a Characteristic
        GattCommunicationStatus status = co_await c.WriteClientCharacteristicConfigurationDescriptorAsync(cccdValue);

        if (status == GattCommunicationStatus::Success)
        {
            c.ValueChanged(Characteristic_ValueChanged);
            std::wcout << L"Successfully subscribed for value changes" << "\n";
        }
        else
        {
            std::wcerr << L"Error registering for value changes" << "\n";
        }
    }
    catch (hresult_access_denied& ex)
    {
        // This usually happens when a device reports that it support indicate, but it actually doesn't.
        std::wcerr << L"Error registering for value changes: Status = " << std::wstring{ ex.message().c_str() };
    }
}

/*
* @brief    Compare two UUIDs
* @param    left:   first UUID
* @param    right:  second UUID
* @return   0/1
*/
int uuid_equal(std::wstring left, guid right)
{
    return (std::wstring(to_hstring(right)).compare(left) == 0);
}

/*
* @brief    Gets all the Characteristics of a service
* @param    service: BLE Service which needs to be searched
* @return   fire_and_forget: because the co_await function needs to run asyncronously
*/
fire_and_forget getCharacteristics(GattDeviceService service) {

    myStatus = STATE_MACHINE::CHARACTERISTICS_SEARCHING;

    //array to store the results
    IVectorView<GattCharacteristic> characteristics{ nullptr };

    try
    {
        // Ensure we have access to the device.
        auto accessStatus = co_await service.RequestAccessAsync();
        if (accessStatus == DeviceAccessStatus::Allowed)
        {
            // BT_Code: Get all the child characteristics of a service. Use the cache mode to specify uncached characterstics only
            GattCharacteristicsResult result = co_await service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached);
            if (result.Status() == GattCommunicationStatus::Success)
            {
                characteristics = result.Characteristics();
            }
            else
            {
                std::wcout << L"Error accessing service. Restarting Process..." << "\n";
                myStatus = STATE_MACHINE::INIT;
            }
        }
        else
        {
            std::wcout << L"Error accessing service. Restarting Process..." << "\n";
            myStatus = STATE_MACHINE::INIT;
        }
    }
    catch (hresult_error& ex)
    {
        std::wcout << L"Restricted service. Can't read characteristics: " << ex.message().c_str();
    }

    if (characteristics)
    {        
        for (GattCharacteristic&& c : characteristics)
        {
            if (powerMateServices[service].find(c) == powerMateServices[service].end())
            {
                //if its the INPUT Characteristic, subscribe to it.
                if (uuid_equal(uuid_input, c.Uuid()))
                {
                    subscribeToValueChange(c);
                }
                powerMateServices[service].insert(c);
            }
            else
            {
                myStatus = STATE_MACHINE::CHARACTERISTICS_FOUND;
            }
        }
    }
}

int main()
{
    //This Command hides the Console Window.
    // ShowWindow(GetConsoleWindow(), SW_HIDE);

    std::cout << "=================================================\n";
    std::cout << "PowerMate Windows Bridge by github.com/livesparks\n";
    std::cout << "Based on https://github.com/s7726/PowerMate \n";
    std::cout << "=================================================\n";

    while (true) {
        switch (myStatus)
        {
        case STATE_MACHINE::INIT:
            std::wcout << "\nStarting the scan for BLE devices.\n";
            StartBleDeviceWatcher();
            break;
        case STATE_MACHINE::DEVICE_SEARCHING:
            break;
        case STATE_MACHINE::DEVICE_FOUND:
            std::wcout << "\nStop the scan for BLE devices.\n";
            StopBleDeviceWatcher();
            selectDevice(powerMateDeviceId);
            break;
        case STATE_MACHINE::DEVICE_SELECTING:
            break;
        case STATE_MACHINE::DEVICE_SELECTED:
            getServices();
            break;
        case STATE_MACHINE::SERVICE_SEARCHING:
            break;
        case STATE_MACHINE::SERVICE_FOUND:
            //for each service of the PowerMate get their characteristics
            for (auto&& service : powerMateServices)
            {
                getCharacteristics(service.first);
                if (myStatus == STATE_MACHINE::INIT)
                {
                    break;
                }
            }
            break;
        case STATE_MACHINE::CHARACTERISTICS_SEARCHING:
            break;
        case STATE_MACHINE::CHARACTERISTICS_FOUND:
            Sleep(500);
            break;
        default:
            break;
        }
        Sleep(500);
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
