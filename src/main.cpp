#include <iostream>
#include <algorithm>
#include "BotEvents.hpp"
#include "InstanceManager.hpp"
#include "VoliServer.hpp"

#include "common.hpp"

using namespace std;
using namespace voli;

int main()
{
	print_header();
	auto service = make_shared<IoService>();

	voli::settings = new voli::Config("config.ini");

	voli::manager = new InstanceManager(service, voli::settings);
	voli::server = new VoliServer(service, 8000);
	voli::database = std::make_shared<Storage>(voli::initStorage("accounts.sqlite"));
	voli::database->sync_schema();

	voli::accounts = voli::database->get_all<Account>();

	int cIdle = 0, cRunning = 0, cFinished = 0;
	for (auto const& account : voli::accounts) {
		switch (account.status) {
		case 0:
			cIdle++;
			break;
		case 1:
			cRunning++;
			if(voli::settings->AutoRunOnStart)
				voli::manager->Start(account);
			break;
		case 2:
			cFinished++;
			break;
		}
	}

	voli::printSystem("Accounts loaded: " + to_string(voli::accounts.size()) + " | Idle: " + to_string(cIdle) + " - Running: " + to_string(cRunning) + " - Finished: " + to_string(cFinished));
	if(cRunning > 0 && voli::settings->AutoRunOnStart)
		voli::printSystem("Auto starting all " + to_string(cRunning) + " Accounts with \"Running\"-State.");

	voli::manager->onevent = {
		// { ".*", voli::OnDebugPrint },
		{ "/plugin-manager/v1/status", voli::OnStatus },
		{ "/lol-matchmaking/v1/search/errors", voli::OnGatekeeperPenalty },
		{ "/lol-pre-end-of-game/v1/currentSequenceEvent", voli::OnPreEndOfGameSequence },
		{ "/lol-summoner/v1/current-summoner", voli::OnCurrentSummoner },
		{ "/lol-login/v1/wallet", voli::OnLoginWallet },
		{ "/lol-honor-v2/v1/reward-granted", voli::OnHonorGranted },
		{ "/lol-gameflow/v1/session", voli::OnGameFlowSession },
		{ "/lol-champ-select/v1/session", voli::OnChampSelectSession },
		{ "/lol-honor-v2/v1/ballot", voli::OnHonorBallot },
		{ "/lol-matchmaking/v1/ready-check", voli::OnMatchmakingReadyCheck },
		{ "/lol-matchmaking/v1/search", voli::OnMatchmakingSearch },
		{ "/lol-leaver-buster/v1/notifications", voli::OnLeaverBusterNotification },
		{ "/lol-login/v1/session", voli::OnLoginSession }
	};

	voli::server->addHandler("RequestInstanceList", [](json data) {
		// Store all Accounts in message.
		json message;
		for (auto const& lol : voli::accounts) {
			message[lol.id] = { {"id", lol.id}, {"status", "Logged out"}, {"summoner", "" } };
		}
		// Replace Accounts that are running with current states.
		for (auto const& lol : voli::manager->GetAll()) {
			json settings = { { "autoPlay", lol.second->account.status },{ "queue", lol.second->account.queueId } };
			message[lol.second->id] = { { "id", lol.second->id },{ "status", lol.second->currentStatus },{ "summoner", lol.second->trashbin["currentSummoner"] },{ "wallet", lol.second->trashbin["wallet"] },{ "settings", settings } };
		}
		voli::server->broadcast(ListInstance{ message });
		return std::string("success");
	});

	voli::server->addHandler("RequestInstanceStart", [](json data) {

		std::string username = data.at("username").get<std::string>();
		std::string password = data.at("password").get<std::string>();
		std::string region = data.at("region").get<std::string>();
		int queueId = data.at("queue").get<int>();
		bool autoplay = data.at("autoplay").get<bool>();

		int autoPlay = (autoplay) ? 1 : 0;
		Account user{ -1, username, password, region, queueId, voli::settings->DefaultMaxLevel, voli::settings->DefaultMaxBE, autoPlay };
		auto insertedId = voli::database->insert(user);
		user.id = insertedId;

		voli::printSystem("Adding new Account (" + user.username + ") atx " + user.region);
		voli::server->broadcast(UpdateStatus{ user.id , "Logged out" });

		voli::accounts.push_back(user);
		voli::manager->Start(user);

		return std::string("success");
	});

	voli::server->addHandler("RequestInstanceLogout", [](json data) {
		int id = data.at("id").get<int>();
		auto x = voli::manager->Get(id);
		if (auto user = voli::database->get_no_throw<Account>(id)) {
			voli::database->remove<Account>(id);
			voli::printSystem("Removing Account (" + user->username + ")");
			voli::server->broadcast(LoggingOut{ user->id });
			voli::accounts.erase(
				std::remove_if(voli::accounts.begin(), voli::accounts.end(), [&](Account const & acc) {
				return acc.id == user->id;
			}),
				voli::accounts.end()
				);
			if (x) {
				auto y = lol::PostProcessControlV1ProcessQuit(*x);
				if (y) {
					voli::print(x->account.username, "Logging out.");
					voli::manager->remove(id);
					return std::string("success");
				}
				return std::string("failed");
			}
			return std::string("success");
		}
		return std::string("failed");
	});

	voli::server->addHandler("RequestChangeSettings", [](json data) {
		int id = data.at("id").get<int>();
		return std::string("success");
	});
	
	voli::server->addHandler("RequestChangeAutoPlay", [](json data) {
		int id = data.at("id").get<int>();
		bool autoPlay = data.at("autoPlay").get<bool>();
		auto x = voli::manager->Get(id);
		if (x) {
			x->account.status = autoPlay;
		}
		return std::string("success");
	});

	voli::printSystem("Access VoliBot Web Manager: http://cdn.volibot.com/bot");
	service->run();
}
