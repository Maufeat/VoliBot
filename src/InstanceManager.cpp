#include <random>
#include <process.hpp>
#include <fstream>
#include <iostream>
#include "InstanceManager.hpp"
using namespace voli;

namespace {
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

InstanceManager::InstanceManager(IoServicePtr service) : mService(service) {

}

void InstanceManager::Start() {
  int port = freePort();
  std::string password = random_string(21);
  auto x = launchWithArgs("C:\\Riot Games\\League of Legends", "--allow-multiple-clients --app-port=" + std::to_string(port) + " --remoting-auth-token=" + password);
  Add(std::make_shared<lol::LeagueClient>("127.0.0.1", port, password));
};


void InstanceManager::Add(std::shared_ptr<lol::LeagueClient> client) {
  int curId = ++nextId;
  client->wss.on_error = [&client](auto c, const SimpleWeb::error_code &e) {
    if (e.value() == 10061)
    {
      std::cout << "Waiting for LeagueClient to be started." << std::endl;
      client->wss.start();
    }
    else
      std::cout << "Client: Error: " + std::to_string(e.value()) + " | error message: " + e.message() << std::endl;
  };
  client->onevent = [this, &curId](auto ci, auto message) {
    onevent(curId, ci, message);
  };
  client->onwelcome = [this, &curId](auto ci) {
    onwelcome(curId, ci);
  };
  mClients[curId] = client;
  client->wss.io_service = mService;
  client->wss.start();
};

std::map<int, std::shared_ptr<lol::LeagueClient>> InstanceManager::GetAll() {
  return mClients;
};

std::shared_ptr<lol::LeagueClient> InstanceManager::Get(int id) {
  return mClients[id];
};
