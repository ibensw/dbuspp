#include "dbuspp/include/dbuspp.hpp"
#include <iostream>

using namespace wibens::dbuspp;

int main() {
    using namespace type;
    DBus dbus;
    auto networkManager = dbus.properties("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager");
    auto devices = networkManager.get<Array<ObjectPath>>("org.freedesktop.NetworkManager", "Devices");

    for (const auto &device : devices) {
        std::cout << "Device:     " << device << std::endl;
        auto nmDevice = dbus.properties("org.freedesktop.NetworkManager", device);
        std::cout << "Interface:  " << nmDevice.get<String>("org.freedesktop.NetworkManager.Device", "Interface")
                  << std::endl;
        std::cout << "MacAddress: " << nmDevice.get<String>("org.freedesktop.NetworkManager.Device", "HwAddress")
                  << std::endl;
        std::cout << "Driver:     " << nmDevice.get<String>("org.freedesktop.NetworkManager.Device", "FirmwareVersion")
                  << std::endl;
        std::cout << "IPv4: " << std::endl;
        auto connection = nmDevice.get<ObjectPath>("org.freedesktop.NetworkManager.Device", "Ip4Config");
        if (connection != "/") {
            std::cout << "Connection: " << connection << std::endl;
            auto v4cfg = dbus.properties("org.freedesktop.NetworkManager", connection);
            auto v4addr =
                v4cfg.get<Array<Dict<String, U32>>>("org.freedesktop.NetworkManager.IP4Config", "AddressData");
            for (const auto &addr : v4addr) {
                std::cout << "Address: " << std::get<String>(addr.at("address")) << "/"
                          << std::get<U32>(addr.at("prefix")) << std::endl;
            }
        }
        std::cout << std::endl << std::endl;
    }
}
