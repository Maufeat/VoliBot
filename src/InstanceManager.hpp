#pragma once
#include "common.hpp"
#include <map>
#include <lol/base_op.hpp>

namespace voli {
  class InstanceManager {
  private:
    int nextId = 0;
    IoServicePtr mService;
    std::map<int, std::shared_ptr<lol::LeagueClient>> mClients;
    void Add(std::shared_ptr<lol::LeagueClient>);
  public:
    InstanceManager(IoServicePtr service);
    std::shared_ptr<lol::LeagueClient> Get(int id);
    std::map<int, std::shared_ptr<lol::LeagueClient>> GetAll();
    void Start();
    std::function<void(int, lol::LeagueClient*)> onwelcome;
    std::function<void(int, lol::LeagueClient*, lol::PluginResourceEvent j)> onevent;
  };
}
