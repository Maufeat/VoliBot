#pragma once
#include "common.hpp"
#include <functional>
#include <unordered_map>
#include <regex>
#include <lol/base_op.hpp>

namespace voli {
  class InstanceManager {
  public:
    using HandlerFunc = std::function<void(lol::LeagueClient&, const std::smatch&, const lol::PluginResourceEventType&, const json&)>;
    using EventMap = std::unordered_multimap<std::string, HandlerFunc>;
  private:
    uint32_t nextId = 0;
    IoServicePtr mService;
    std::unordered_map<uint32_t, std::shared_ptr<lol::LeagueClient>> mClients;
    void Add(std::shared_ptr<lol::LeagueClient>);
  public:
    InstanceManager(IoServicePtr service);
    std::shared_ptr<lol::LeagueClient> Get(uint32_t id) const;
    const std::unordered_map<uint32_t, std::shared_ptr<lol::LeagueClient>>& GetAll() const;
    void remove(uint32_t id);
    void Start();
    std::function<void(lol::LeagueClient&)> onwelcome;
    std::function<void(uint32_t id)> onclose;
    EventMap onevent;
  };
}
