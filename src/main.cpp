#include <iostream>
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

	voli::manager = new InstanceManager(service);
	voli::server = new VoliServer(service, 8000);
	voli::database = std::make_shared<Storage>(voli::initStorage("accounts.sqlite"));
	voli::database->sync_schema();

	// GET ACCOUNTS
	/*
	voli::accounts = voli::database->get_all<Account>();
	cout << "allUsers (" << voli::accounts.size() << "):" << endl;
	for (auto &user : voli::accounts) {
		cout << voli::database->dump(user) << endl; //  dump returns std::string with json-like style object info. For example: { id : '1', first_name : 'Jonh', last_name : 'Doe', birth_date : '664416000', image_url : 'https://cdn1.iconfinder.com/data/icons/man-icon-set/100/man_icon-21-512.png', type_id : '3' }
	}*/

	// INSERT ACCOUNT
	/*
	Account user{ -1, "Jonh", "Doe", "EUW", 30, 30000, "new" };

	auto insertedId = voli::database->insert(user);
	cout << "insertedId = " << insertedId << endl;      //  insertedId = 8
	user.id = insertedId;
	*/

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
		auto x = voli::manager->GetAll();
		json message;
		for (auto const& lol : x) {
			json settings = { { "autoPlay", lol.second->autoplay },{ "queue", lol.second->queue } };
			message[lol.second->id] = { {"id", lol.second->id}, {"status", lol.second->currentStatus}, {"summoner", lol.second->trashbin["currentSummoner"] }, {"wallet", lol.second->trashbin["wallet"]}, {"settings", settings } };
		}
		voli::server->broadcast(ListInstance{ message });
		return std::string("success");
	});

	voli::server->addHandler("RequestInstanceStart", [](json data) {
		std::string username = data.at("username").get<std::string>();
		std::string password = data.at("password").get<std::string>();
		std::string region = data.at("region").get<std::string>();
		int queue = data.at("queue").get<int>();
		bool autoplay = data.at("autoplay").get<bool>();
		voli::printSystem("Adding new Account (" + username + ") at " + region);
		voli::manager->Start(username, password, region, queue, autoplay);
		return std::string("success");
	});

	voli::server->addHandler("RequestInstanceLogout", [](json data) {
		int id = data.at("id").get<int>();
		auto x = voli::manager->Get(id);
		if (x) {
			auto y = lol::PostProcessControlV1ProcessQuit(*x);
			if (y) {
				voli::server->broadcast(LoggingOut{ x->id });
				voli::print(x->lolUsername, "Logging out.");
				voli::printSystem("Removing Account (" + x->lolUsername + ")");
				voli::manager->remove(id);
				return std::string("success");
			}
			return std::string("failed");
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
			x->autoplay = true;
		}
		return std::string("success");
	});

	voli::printSystem("Access VoliBot Web Manager: http://cdn.volibot.com/bot");
	service->run();
}
