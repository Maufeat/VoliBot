#include <random>
#include <process.hpp>
#include <fstream>
#include <iostream>
#include "InstanceManager.hpp"

using namespace voli;

namespace voli {
  std::string get_exe(std::string path) {
    if (std::ifstream(path + "/LeagueClient.bat")) {
      return "\\LeagueClient.bat";
    }
    else if (std::ifstream(path + "/LeagueClient.exe")) {
      return "\\LeagueClient.exe";
    }
    else if (std::ifstream(path + "/LeagueClient.app")) {
      return "\\LeagueClient.app";
    }
    return "";
  };

  long launchWithArgs(std::string path, std::string args)
  {
    TinyProcessLib::Process lcuProcess(path + get_exe(path) + " " + args);
    auto exit_status = lcuProcess.get_exit_status();
    return lcuProcess.get_id();
  };

  uint16_t freePort()
  {
    asio::io_service service;
    asio::ip::tcp::acceptor acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    return acceptor.local_endpoint().port();
  }

  std::string random_string(size_t length)
  {
    static auto& chrs = "0123456789"
      "abcdefghijklmnopqrstuvwxyz-"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

    thread_local static std::mt19937 rg{ std::random_device{}() };
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while (length--)
      s += chrs[pick(rg)];

    return s;
  };

}

InstanceManager::InstanceManager(IoServicePtr service, Config* settings) : mService(service), settings(settings) {

}

void InstanceManager::Start(voli::Account user) {
  int port = freePort();
  std::string password = random_string(21);
  auto x = launchWithArgs(settings->LeaguePath, "--allow-multiple-clients --app-port=" + std::to_string(port) + " --remoting-auth-token=" + password);
  if (x != 0) {
	  std::shared_ptr<voli::LeagueInstance> client = std::make_shared<voli::LeagueInstance>("127.0.0.1", port, password);
	  client->id = user.id;
	  client->account = user;
	  Add(client);
  }
  else
	  voli::printSystem("No League found. Please check your League of Legends Path in Settings.");
};


void InstanceManager::Add(std::shared_ptr<voli::LeagueInstance> client) {
  std::weak_ptr<voli::LeagueInstance> ptr = client;
  mClients[client->account.id] = client;
  client->wss.on_error = [ptr](auto c, const SimpleWeb::error_code &e) {
    if (auto spt = ptr.lock()) {
      if (e.value() == 10061)
      {
        std::cout << "Waiting for LeagueClient to be started." << std::endl;
        spt->wss.start();
      }
	  else if(e.value() == 10009) {}
      else
        std::cout << "Client: Error: " + std::to_string(e.value()) + " | error message: " + e.message() << std::endl;
    }
  };
  client->wss.on_message = [this, ptr](std::shared_ptr<lol::WssClient::Connection> connection,
      std::shared_ptr<lol::WssClient::Message> message) {
    if(auto spt = ptr.lock()) {
      if (message->size() > 0) {
        auto j = json::parse(message->string()).get<std::vector<json>>();
        if (j[0].get<int32_t>() == 0) {
          auto send_stream = std::make_shared <lol::WssClient::SendStream >();
          *send_stream << "[5, \"OnJsonApiEvent\"]";
          connection->send(send_stream);
          if (onwelcome)
            onwelcome(*spt);
        }
        else if (j[0].get<int32_t>() == 8 && j[1].get<std::string>() == "OnJsonApiEvent") {
          auto pevent = j[2].get<lol::PluginResourceEvent>();
          for (const auto& p : onevent)
            if (std::smatch match; std::regex_match(pevent.uri, match, std::regex(p.first)))
              p.second(*spt, match, pevent.eventType, pevent.data);
        }
      }
    }
  };
  client->wss.io_service = mService;
  client->httpsa.io_service = mService;
  client->wss.start();
};

const std::unordered_map<uint32_t, std::shared_ptr<voli::LeagueInstance>>& InstanceManager::GetAll() const {
  return mClients;
};

std::shared_ptr<voli::LeagueInstance> InstanceManager::Get(uint32_t id) const {
  auto it = mClients.find(id);
  return it == mClients.end() ? nullptr : it->second;
};

void InstanceManager::remove(uint32_t id) {
  mClients.erase(id);
}
