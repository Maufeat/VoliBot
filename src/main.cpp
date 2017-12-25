#include <iostream>
#include "InstanceManager.hpp"
#include "VoliServer.hpp"

#include <lol/def/PluginManagerResource.hpp>
#include <lol/def/LolHonorV2Ballot.hpp>
#include <lol/def/LolChampSelectChampSelectSession.hpp>
#include <lol/def/LolSummonerSummoner.hpp>

#include <lol/op/AsyncStatus.hpp>
#include <lol/op/PostLolLoginV1Session.hpp>
#include <lol/op/GetLolMatchmakingV1Search.hpp>
#include <lol/op/GetLolLobbyV2Lobby.hpp>
#include <lol/op/PostLolLobbyV2Lobby.hpp>
#include <lol/op/PostLolLobbyV2LobbyMatchmakingSearch.hpp>
#include <lol/op/PostLolMatchmakingV1ReadyCheckAccept.hpp>
#include <lol/op/PostLolHonorV2V1HonorPlayer.hpp>
#include <lol/op/PostLolLobbyV2PlayAgain.hpp>
#include <lol/op/GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId.hpp>

#include "common.hpp"

using namespace std;
using namespace voli;

struct ExampleEvent {
  static constexpr const auto NAME = "ExampleEvent";
  std::string message;
};

static void to_json(json& j, const ExampleEvent& v) {
  j["message"] = v.message;
}

int main()
{
	print_header();
	auto service = make_shared<IoService>();
	InstanceManager manager(service);
	VoliServer server(service, 8000);
	manager.Start();
	manager.onevent.emplace(".*", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
	});
	manager.onevent.emplace("/plugin-manager/v1/status",
		[](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::PluginManagerResource& data) {
		if (data.state == lol::PluginManagerState::PluginsInitialized_e) {
			auto res = lol::PostLolLoginV1Session(c, { "", "" });
			if (res) {
				if (res.data->error) {
					if (res.data->error->messageId == "INVALID_CREDENTIALS") {
						voli::print("Test", "Wrong credentials.");
						c.wss.stop();
					}
				}
			}
		}
	});

	// lol-summoner/v1/current-summoner
	manager.onevent.emplace("/lol-summoner/v1/current-summoner", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			c.trashbin["currentSummoner"] = data;
			break;
		}
	});
	// /lol-champions/v1/inventories/{summonerId}/champions/{championId}
	manager.onevent.emplace("/lol-champ-select/v1/session", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Update_e:
			break;
		case lol::PluginResourceEventType::Create_e:
			lol::LolChampSelectChampSelectSession champSelectSession = data;
			for (auto const& player : champSelectSession.myTeam) {
				if (player.cellId == champSelectSession.localPlayerCellId) {
					auto championInfos = lol::GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId(c, player.summonerId, player.championId);
					voli::print("Test", "We've got: " + championInfos.data->name);
				}
			}
			break;
		}
	});

	manager.onevent.emplace("/data-store/v1/install-settings/gameflow-process-info", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			voli::print("Test", "Starting League of Legends.");
			break;
		}
	});

	// lol-honor-v2/v1/ballot
	manager.onevent.emplace("/lol-honor-v2/v1/ballot", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolHonorV2Ballot ballot = data;
			lol::LolHonorV2ApiHonorPlayerServerRequest honorRequest;
			voli::print("Test", "Finished game.");
			auto randomPlayer = ballot.eligiblePlayers[random_number(ballot.eligiblePlayers.size() - 1)];
			honorRequest.gameId = ballot.gameId;
			honorRequest.summonerId = randomPlayer.summonerId;
			honorRequest.honorCategory = "HEART";
			voli::print("Test", "We are honoring: " + randomPlayer.skinName);
			lol::PostLolHonorV2V1HonorPlayer(c, honorRequest);
			// after honoring, we are queueing again. Because we are in end-of-game screen and we are now execute the "play again" button.
			auto x = lol::PostLolLobbyV2PlayAgain(c);
			if (!x.error) {
				auto lobby = lol::GetLolLobbyV2Lobby(c);
				if (lobby.data->canStartActivity == true) {
					voli::print("Test", "Joining ARAM Queue.");
					lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
				}
			}
			break;
		}
	});

	// /lol-matchmaking/v1/ready-check
	manager.onevent.emplace("/lol-matchmaking/v1/ready-check", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolMatchmakingMatchmakingReadyCheckResource readyCheckResource = data;
			switch (readyCheckResource.state) {
			case lol::LolMatchmakingMatchmakingReadyCheckState::StrangerNotReady_e:
				voli::print("Test", "Someone hasn't accepted.");
				break;
			case lol::LolMatchmakingMatchmakingReadyCheckState::InProgress_e:
				if (readyCheckResource.playerResponse == lol::LolMatchmakingMatchmakingReadyCheckResponse::None_e)
					voli::print("Test", "Ready Check accepted.");
					auto res = lol::PostLolMatchmakingV1ReadyCheckAccept(c);
				break;
			}
			break;
		}
	});

	// /lol-matchmaking/v1/search
	manager.onevent.emplace("/lol-matchmaking/v1/search", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolMatchmakingMatchmakingSearchResource searchResource = data;
			switch (searchResource.searchState) {
			case lol::LolMatchmakingMatchmakingSearchState::Found_e:
				c.trashbin.erase("searchQueue");
				break;
			case lol::LolMatchmakingMatchmakingSearchState::Searching_e:
				const auto& lastQueuePos = c.trashbin.find("searchQueue");
				if (lastQueuePos != c.trashbin.end())
					break;
				c.trashbin["searchQueue"] = searchResource;
				voli::print("Test", "In Queue (Estimated Queue Time: " + to_string((int)searchResource.estimatedQueueTime) + "s)");
				break;
			}
		}
	});

	// /lol-login/v1/session
	manager.onevent.emplace("/lol-login/v1/session", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolLoginLoginSession loginSession = data;
			switch (loginSession.state) {
			case lol::LolLoginLoginSessionStates::IN_PROGRESS_e:
			{
				if (loginSession.queueStatus) {
					const auto& lastQueuePos = c.trashbin.find("queuePos");
					if (lastQueuePos != c.trashbin.end())
						if (lastQueuePos->second.at("estimatedPositionInQueue").get<uint64_t>() == loginSession.queueStatus->estimatedPositionInQueue)
							break;
					c.trashbin["queuePos"] =  loginSession.queueStatus;
					voli::print("Test", "Logging in. (Queue Position: " + to_string(loginSession.queueStatus->estimatedPositionInQueue) + " - " + *lol::to_string(loginSession.queueStatus->approximateWaitTimeSeconds) + "s)");
				}
				break;
			}
			case lol::LolLoginLoginSessionStates::SUCCEEDED_e:
				if (loginSession.connected) {
					voli::print("Test", "Successfully logged in.");
					auto &lobbyConfig = lol::LolLobbyLobbyChangeGameDto();
					lobbyConfig.queueId = 450;
					auto res = lol::PostLolLobbyV2Lobby(c, lobbyConfig);
					if (res) {
						if (res.data->canStartActivity == true) {
							voli::print("Test", "Joining ARAM Queue.");
							lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
						}
					}
				}
				break;
			case lol::LolLoginLoginSessionStates::LOGGING_OUT_e:
				voli::print("Test", "Logging out.");
				c.wss.stop();
				break;
			}
			break;
		}
	});
	server.addHandler("test", [&server](json data) {
		server.broadcast(ExampleEvent{ "haha" });
		return std::string("success");
	});
	service->run();
}
