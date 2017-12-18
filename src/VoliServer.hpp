#pragma once
#include <map>
#include <functional>
#include <memory>
#include "common.hpp"
#include "message/MessageType.hpp"

namespace voli {
  class VoliServer {
  private:
    using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
    WsServer mServer;
    IoServicePtr mService;
    std::map<std::string, std::function<json(const json& data)>> mHandlers;
  public:
    VoliServer(IoServicePtr service, int port);
    void broadcast(const std::string& name, const nlohmann::json& data);
    void addHandler(const std::string& name, std::function<json(const json&)> fun);

    template<typename T>
    void broadcast(const T& data) {
      broadcast(T::NAME, data);
    }
  };
}
