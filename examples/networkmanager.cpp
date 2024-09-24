#include "dbuspp/include/dbuspp.hpp"
#include <iostream>

using namespace wibens::dbuspp;

int main() noexcept(false) {
    using namespace type;
    DBus dbus;

    auto networkManager = Properties{&dbus, "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager"};
    auto devices = networkManager.get<Array<ObjectPath>>("org.freedesktop.NetworkManager", "Devices");

    for (const auto &device : devices) {
        auto nmDevice = Properties{&dbus, "org.freedesktop.NetworkManager", device};
        std::cout << "Interface:  " << nmDevice.get<String>("org.freedesktop.NetworkManager.Device", "Interface")
                  << std::endl;
        std::cout << "MacAddress: " << nmDevice.get<String>("org.freedesktop.NetworkManager.Device", "HwAddress")
                  << std::endl;
        static constexpr auto properies = {std::make_pair("Ip4Config", "org.freedesktop.NetworkManager.IP4Config"),
                                           std::make_pair("Ip6Config", "org.freedesktop.NetworkManager.IP6Config")};
        for (const auto [property, interface] : properies) {
            auto connection = nmDevice.get<ObjectPath>("org.freedesktop.NetworkManager.Device", property);
            if (connection != "/") {
                auto peer = Peer{&dbus, "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager"};
                auto v4cfg = Properties{&dbus, "org.freedesktop.NetworkManager", connection};
                auto v4addr = v4cfg.get<Array<Dict<String, Variant<String, U32>>>>(interface, "AddressData");
                for (const auto &addr : v4addr) {
                    std::cout << "Address:    " << std::get<String>(addr.at("address")) << "/"
                              << std::get<U32>(addr.at("prefix")) << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
}
