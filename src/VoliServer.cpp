#include "VoliServer.hpp"
using namespace voli;

VoliServer::VoliServer(IoServicePtr service, int port) {
  mServer.config.port = port;
  mServer.io_service = service;
  mServer.endpoint["^/volibot/?$"].on_message = [this](std::shared_ptr<WsServer::Connection> connection,
    std::shared_ptr<WsServer::Message> message) {
    const auto& request = message->string();
    auto send_stream = std::make_shared<WsServer::SendStream>();
    try {
      std::array<json, 4> j = json::parse(request);
      const auto& reqtype = j[0].get<MessageType>();
      const auto& reqid = j[1].get<std::string>();
      const auto& reqname = j[2].get<std::string>();
      const auto& reqdata = j[3];
      if (auto handler = mHandlers.find(reqname); handler != mHandlers.end())
        try {
        const auto& result = handler->second(reqdata);
        *send_stream << json({ MessageType::RESPONSE_SUCCESS, reqid,  result });
      }
      catch (const std::exception &exc) {
        *send_stream << json({ MessageType::RESPONSE_ERROR, reqid, std::string(exc.what()) });
      }
      else
        *send_stream << json({ MessageType::RESPONSE_ERROR, reqid, std::string("No handler!") });
    }
    catch (const std::exception& exc) {
      *send_stream << json({ MessageType::MESSAGE_ERROR, request, exc.what() });
    }
    connection->send(send_stream);
  };
  mServer.start();
}

void VoliServer::broadcast(const std::string& name, const json& data) {
  auto send_stream = std::make_shared<WsServer::SendStream>();
  *send_stream << json({ MessageType::EVENT, name, data });
  for (auto& c : mServer.get_connections())
    c->send(send_stream);
}

void VoliServer::addHandler(const std::string& name, std::function<json(const json&)> fun) {
  mHandlers[name] = fun;
}

