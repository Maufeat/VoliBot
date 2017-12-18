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
	std::map<int, std::shared_ptr<lol::LeagueClient>> GetAll();
	void Start(int port, std::string password);
	std::function<void(int, lol::LeagueClient*)> onwelcome;
	std::function<void(int, lol::LeagueClient*, lol::PluginResourceEvent j)> onevent;

};

void InstanceManager::Start(int port, std::string password) {
	//Add(std::make_shared<lol::LeagueClient>("127.0.0.1", Utils::freePort(), Utils::random_string(21)));
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
	clients[curId] = client;

		client->wss.start();
};

std::map<int, std::shared_ptr<lol::LeagueClient>> InstanceManager::GetAll() {
	return clients;
};

std::shared_ptr<lol::LeagueClient> InstanceManager::Get(int id) {
	// Create a check, if exists in map
	return clients[id];
};
