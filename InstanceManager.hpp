#pragma once

#include "Utils.hpp"
#include <lol/base_op.hpp>

class InstanceManager {

private:
	int nextId = 0;
	std::map<int, std::shared_ptr<lol::LeagueClient>> clients;
	void Add(std::shared_ptr<lol::LeagueClient>);

public:
	std::shared_ptr<lol::LeagueClient> Get(int id);
	void Start(nlohmann::json req);
	std::function<void(int, lol::LeagueClient*)> onwelcome;
	std::function<void(int, lol::LeagueClient*, lol::PluginResourceEvent j)> onevent;

};

void InstanceManager::Start(nlohmann::json req) {
	Add(std::make_shared<lol::LeagueClient>("127.0.0.1", Utils::freePort(), Utils::random_string(21)));
};

void InstanceManager::Add(std::shared_ptr<lol::LeagueClient> client) {
	int curId = nextId;
	client->onevent = [&curId](auto ci, auto message) {
		onevent(curId, ci, message);
	};
	client->onwelcome = [&curId](auto ci) {
		clients[curId] = client;
		onwelcome(curId, ci);
	};
	nextId++;
};

std::shared_ptr<lol::LeagueClient> InstanceManager::Get(int id) {
	// Create a check, if exists in map
	return clients[id];
};
