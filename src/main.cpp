#include <iostream>
#include "BotEvents.hpp"
#include "InstanceManager.hpp"
#include "VoliServer.hpp"
#include "../sqlite/sqlite_orm.h"

#include <lol/def/PluginManagerResource.hpp>
#include <lol/def/LolHonorV2Ballot.hpp>
#include <lol/def/LolChampSelectChampSelectSession.hpp>
#include <lol/def/LolSummonerSummoner.hpp>
#include <lol/def/LolEndOfGameEndOfGameStats.hpp>
#include <lol/def/LolPreEndOfGameSequenceEvent.hpp>
#include <lol/def/LolGameflowGameflowSession.hpp>
#include <lol/def/LeaverBusterNotificationResource.hpp>
#include <lol/def/LolMatchmakingMatchmakingSearchErrorResource.hpp>

#include <lol/op/AsyncStatus.hpp>
#include <lol/op/PostLolLoginV1Session.hpp>
#include <lol/op/PostLolLobbyV2Lobby.hpp>
#include <lol/op/PostLolLobbyV2LobbyMatchmakingSearch.hpp>
#include <lol/op/PostLolMatchmakingV1ReadyCheckAccept.hpp>
#include <lol/op/PostLolHonorV2V1HonorPlayer.hpp>
#include <lol/op/PostLolLobbyV2PlayAgain.hpp>
#include <lol/op/PostLolSummonerV1Summoners.hpp>
#include <lol/op/GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId.hpp>
#include <lol/op/PostPatcherV1ProductsByProductIdStartPatchingRequest.hpp>
#include <lol/op/GetLolMatchmakingV1Search.hpp>
#include <lol/op/GetLolLobbyV2Lobby.hpp>
#include <lol/op/GetLolLoginV1Wallet.hpp>
#include <lol/op/GetLolSummonerV1CurrentSummoner.hpp>
#include <lol/op/GetLolChampSelectV1PickableChampions.hpp>
#include <lol/op/PutLolSummonerV1CurrentSummonerIcon.hpp>
#include <lol/op/PostLolLoginV1NewPlayerFlowCompleted.hpp>
#include <lol/op/DeleteLolLeaverBusterV1NotificationsById.hpp>
#include <lol/op/PostLolChampSelectV1SessionActionsByIdComplete.hpp>
#include <lol/op/PostLolGameflowV1Reconnect.hpp>
#include <lol/op/GetLolEndOfGameV1EogStatsBlock.hpp>
#include <lol/op/PostLolGameflowV1PreEndGameTransition.hpp>
#include <lol/op/PatchLolChampSelectV1SessionActionsById.hpp>
#include <lol/op/GetPatcherV1ProductsByProductIdState.hpp>
#include <lol/op/GetRiotclientGetRegionLocale.hpp>
#include <lol/op/PostProcessControlV1ProcessQuit.hpp>
#include <lol/op/PostRiotclientSetRegionLocale.hpp>
#include <lol/op/PostLolPreEndOfGameV1CompleteBySequenceEventName.hpp>
#include <lol/op/PostLolMissionsV1MissionsUpdate.hpp>
#include <lol/op/GetLolLeaverBusterV1Notifications.hpp>
#include <lol/op/DeleteLolLeaverBusterV1NotificationsById.hpp>
#include <lol/op/GetLolMatchmakingV1SearchErrors.hpp>

#include "common.hpp"

using namespace std;
using namespace voli;

int main()
{
	print_header();
	auto service = make_shared<IoService>();

	voli::manager = new InstanceManager(service);
	voli::server = new VoliServer(service, 8000);

	auto storage = sqlite_orm::make_storage("accounts.sqlite");
	storage.sync_schema();
	std::cout << storage.libversion() << std::endl;

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
