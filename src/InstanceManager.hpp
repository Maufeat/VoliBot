#pragma once
#include "common.hpp"
#include <functional>
#include <unordered_map>
#include <regex>
#include <lol/base_op.hpp>

namespace voli {

  class InstanceManager {
  public:
    using HandlerFunc = std::function<void(voli::LeagueInstance&, const std::smatch&, lol::PluginResourceEventType, const json&)>;
    using EventMap = std::unordered_multimap<std::string, HandlerFunc>;
  private:
    uint32_t nextId = 0;
    IoServicePtr mService;
	voli::Config *settings;
    std::unordered_map<uint32_t, std::shared_ptr<voli::LeagueInstance>> mClients;
    void Add(std::shared_ptr<voli::LeagueInstance>);
  public:
    InstanceManager(IoServicePtr service, voli::Config* settings);
    std::shared_ptr<voli::LeagueInstance> Get(uint32_t id) const;
    const std::unordered_map<uint32_t, std::shared_ptr<voli::LeagueInstance>>& GetAll() const;
    void remove(uint32_t id);
	void Start(voli::Account user);
    std::function<void(voli::LeagueInstance&)> onwelcome;
    std::function<void(uint32_t id)> onclose;
    EventMap onevent;
  };
}
