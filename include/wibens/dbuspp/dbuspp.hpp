#pragma once

#include "types.hpp"
#include <chrono>
#include <dbus-1.0/dbus/dbus.h>
#include <memory>
#include <string>
#include <string_view>

namespace wibens::dbuspp {
inline namespace v1 {

class DBus {
  public:
    using MessageType = std::unique_ptr<DBusMessage, void (*)(DBusMessage *)>;
    class Message {
      public:
        Message(std::string_view bus, std::string_view path, std::string_view interface, std::string_view method)
            : msg({dbus_message_new_method_call(bus.data(), path.data(), interface.data(), method.data()),
                   dbus_message_unref}) {
            if (msg == nullptr) {
                throw std::runtime_error("Could not create dbus message");
            }
        }

        template <typename... Args>
        Message(std::string_view bus, std::string_view path, std::string_view interface, std::string_view method,
                Args... args)
            : Message(bus, path, interface, method) {
            addArguments(args...);
        }

        template <typename T, typename... TYPES> void addArguments(T first, TYPES... args) {
            addArgument(first);
            if constexpr (sizeof...(args) > 0) {
                addArguments(args...);
            }
        }

        template <typename T> inline void addArgument(const T &arg) { parsers::Parser<T>::addArgument(msg.get(), arg); }

        [[nodiscard]] inline DBusMessage *get() const { return msg.get(); }

      private:
        MessageType msg;

        inline void addArgument(std::string_view arg) {
            auto temp = arg.data();
            dbus_message_append_args(msg.get(), DBUS_TYPE_STRING, &temp, DBUS_TYPE_INVALID);
        }
        inline void addArgument(uint32_t arg) {
            dbus_message_append_args(msg.get(), DBUS_TYPE_UINT32, &arg, DBUS_TYPE_INVALID);
        }
    };
    class Reply {
      public:
        Reply(DBusMessage *replyPtr) : reply({replyPtr, dbus_message_unref}) {
            if (dbus_message_get_type(reply.get()) != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
                throw std::runtime_error("Invalid reply type");
            }
        }

        template <typename T> auto response() {
            DBusMessageIter iter;
            if (!dbus_message_iter_init(reply.get(), &iter)) {
                throw std::runtime_error("Could not create dbus message iterator");
            }
            return parsers::Parser<T>::parse(&iter);
        }

      private:
        MessageType reply;
    };

    DBus() {
        DBusError err;
        dbus_error_init(&err);

        conn = ConnectionType(dbus_bus_get(DBUS_BUS_SYSTEM, &err), dbus_connection_unref);
        if (dbus_error_is_set(&err)) {
            throw std::runtime_error("Could not connect to dbus");
        }
    }

    template <typename... Args>
    [[nodiscard]] inline Message makeMessage(std::string_view destination, std::string_view objectPath,
                                             std::string_view interface, std::string_view method, Args... args) {
        return {destination, objectPath, interface, method, args...};
    }
    Reply sendMessage(const Message &msg, std::chrono::milliseconds timeout = std::chrono::milliseconds(-1)) {
        DBusError err;
        dbus_error_init(&err);
        auto reply = dbus_connection_send_with_reply_and_block(conn.get(), msg.get(), timeout.count(), &err);
        if (dbus_error_is_set(&err)) {
            throw std::runtime_error(std::string{"DBUS Message failed: "} + err.message);
        }
        return reply;
    }

    template <typename T = void, typename... ARGS>
    T call(std::string_view destination, std::string_view objectPath, std::string_view interface,
           std::string_view method, ARGS... args) {
        auto msg = makeMessage(destination, objectPath, interface, method, args...);
        auto reply = sendMessage(msg);
        if constexpr (!std::is_same_v<T, void>) {
            return reply.template response<T>();
        }
    }

  private:
    using ConnectionType = std::unique_ptr<DBusConnection, void (*)(DBusConnection *)>;
    ConnectionType conn{nullptr, dbus_connection_unref};
};

class Interface {
  public:
    Interface(DBus *dbus, std::string destination, std::string path)
        : dbus(dbus), destination(std::move(destination)), path(std::move(path)) {}

  protected:
    DBus *dbus;
    std::string destination;
    std::string path;
};

class Properties : public Interface {
  public:
    using Interface::Interface;

    template <typename T> [[nodiscard]] T get(std::string_view interface, std::string_view property) {
        return std::get<T>(dbus->call<std::variant<T>>(destination, path, DBUS_PROPS, "Get", interface, property));
    }

    template <typename T> [[nodiscard]] type::Dict<type::String, T> getAll(std::string_view interface) {
        return dbus->call<type::Dict<type::String, T>>(destination, path, DBUS_PROPS, "GetAll", interface);
    }

    template <typename T> void set(std::string_view interface, std::string_view property, const T &value) {
        dbus->call<void>(destination, path, DBUS_PROPS, "Set", interface, property, value);
    }

  private:
    static inline constexpr std::string_view DBUS_PROPS = "org.freedesktop.DBus.Properties";
};

class Introspectable : public Interface {
  public:
    using Interface::Interface;

    type::String introspect() {
        return dbus->call<std::string>(destination, path, "org.freedesktop.DBus.Introspectable", "Introspect");
    }
};

class Peer : public Interface {
  public:
    using Interface::Interface;

    void ping() { return dbus->call<void>(destination, path, "org.freedesktop.DBus.Peer", "Ping"); }
    type::String getMachineId() {
        return dbus->call<type::String>(destination, path, "org.freedesktop.DBus.Peer", "GetMachineId");
    }
};

class ObjectManager : public Interface {
  public:
    using Interface::Interface;

    auto getManagedObjects() {
        using CallType = type::Dict<type::ObjectPath, type::Dict<type::String, type::Dict<type::String, type::Ignore>>>;
        return dbus->call<CallType>(destination, path, "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
};

} // namespace v1
} // namespace wibens::dbuspp
