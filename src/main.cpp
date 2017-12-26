#include <iostream>
#include "InstanceManager.hpp"
#include "VoliServer.hpp"

#include <lol/def/PluginManagerResource.hpp>
#include <lol/def/LolHonorV2Ballot.hpp>
#include <lol/def/LolChampSelectChampSelectSession.hpp>
#include <lol/def/LolSummonerSummoner.hpp>

#include <lol/op/AsyncStatus.hpp>
#include <lol/op/PostLolLoginV1Session.hpp>
#include <lol/op/PostLolLobbyV2Lobby.hpp>
#include <lol/op/PostLolLobbyV2LobbyMatchmakingSearch.hpp>
#include <lol/op/PostLolMatchmakingV1ReadyCheckAccept.hpp>
#include <lol/op/PostLolHonorV2V1HonorPlayer.hpp>
#include <lol/op/PostLolLobbyV2PlayAgain.hpp>
#include <lol/op/PostLolSummonerV1Summoners.hpp>
#include <lol/op/GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId.hpp>
#include <lol/op/GetLolMatchmakingV1Search.hpp>
#include <lol/op/GetLolLobbyV2Lobby.hpp>
#include <lol/op/GetLolSummonerV1CurrentSummoner.hpp>
#include <lol/op/GetLolChampSelectV1PickableChampions.hpp>
#include <lol/op/PutLolSummonerV1CurrentSummonerIcon.hpp>
#include <lol/op/PostLolLoginV1NewPlayerFlowCompleted.hpp>
#include <lol/op/DeleteLolLeaverBusterV1NotificationsById.hpp>
#include <lol/op/PostLolChampSelectV1SessionActionsByIdComplete.hpp>
#include <lol/op/PatchLolChampSelectV1SessionActionsById.hpp>

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
	manager.onevent.emplace(".*", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
	});

	/**
	* This says that our client is fully initialized and we can start
	* to send our login request. Here we have to handle wrong credentials
	* and probably ban messages and then removing the client from the
	* InstanceManager.
	*/
	manager.onevent.emplace("/plugin-manager/v1/status",
		[](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::PluginManagerResource& data) {
		if (data.state == lol::PluginManagerState::PluginsInitialized_e) {
			auto res = lol::PostLolLoginV1Session(c, { c.lolUsername, c.lolPassword });
			if (res) {
				if (res.data->error) {
					if (res.data->error->messageId == "INVALID_CREDENTIALS") {
						voli::print(c.lolUsername, "Wrong credentials.");
						c.wss.stop();
					}
				}
			}
		}
	});

	/**
	* Saves our current summoner into trashbin. We can and should probably
	* just request this endpoint when needed instead of storing it. But
	* this has to be tested if this here fails, if not we can leave this.
	*/
	manager.onevent.emplace("/lol-summoner/v1/current-summoner", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			c.trashbin["currentSummoner"] = data;
			break;
		}
	});

	/**
	* Handling the champion selection screen. Currently we are getting the
	* pickable champions list and choose a random champion from that list
	* which should work in every queue. As for ARAM, we check if the actions
	* size is 0, because ARAM requires no actions. Here we should also handle
	* trading requests in ARAM, to give Human Players prefered champions.
	*/
	manager.onevent.emplace("/lol-champ-select/v1/session", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Update_e:
			break;
		case lol::PluginResourceEventType::Create_e:
			lol::LolChampSelectChampSelectSession champSelectSession = data;
			if (champSelectSession.actions.size() == 0) {
				for (auto const& player : champSelectSession.myTeam) {
					if (player.cellId == champSelectSession.localPlayerCellId && player.championId > 0) {
						auto championInfos = lol::GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId(c, player.summonerId, player.championId);
						voli::print(c.lolUsername, "We've got: " + championInfos.data->name);
					}
				}
			}
			else {
				for (auto const& action : champSelectSession.actions) {
					for (auto const& aaction : action) {
						if (aaction.at("actorCellId").get<uint32_t>() == champSelectSession.localPlayerCellId && aaction.at("completed").get<bool>() == false) {
							if (aaction.at("type").get<std::string>() == "pick") {
								auto pickable = lol::GetLolChampSelectV1PickableChampions(c);
								auto randomId = pickable->championIds[random_number(pickable->championIds.size() - 1)];
								lol::LolSummonerSummoner summonerInfo = c.trashbin["currentSummoner"];
								auto championInfos = lol::GetLolChampionsV1InventoriesBySummonerIdChampionsByChampionId(c, summonerInfo.summonerId, randomId);
								voli::print(c.lolUsername, "We are picking: " + championInfos->name);
								lol::LolChampSelectChampSelectAction csAction;
								csAction.actorCellId = champSelectSession.localPlayerCellId;
								csAction.championId = randomId;
								csAction.completed = false;
								csAction.type = "pick";
								csAction.id = aaction.at("id").get<uint32_t>();
								lol::PatchLolChampSelectV1SessionActionsById(c, csAction.id, csAction);
								lol::PostLolChampSelectV1SessionActionsByIdComplete(c, csAction.id);
							}
							else if (aaction.at("type").get<std::string>() == "ban")
								voli::print(c.lolUsername, "We have to ban.");
						}
					}
				}
			}
			break;
		}
	});

	/**
	* Gets triggered when the League Of Legends Client is started. There
	* should be also another way to get this done with checks and probably
	* a way to reconnect to the game when disconnected.
	*/
	manager.onevent.emplace("/data-store/v1/install-settings/gameflow-process-info", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			// TODO: Here we have to probably trigger reconnect.
			voli::print(c.lolUsername, "Starting League of Legends.");
			break;
		}
	});
	
	/**
	* We are using the Honor System to check if the game is over. We should
	* probably try to find another way to re-join the queue. But as for now 
	* this looks okay since it is getting triggered on every PVP EOG screen.
	* We also have to change the queueId here when reaching Level 6.
	*/
	manager.onevent.emplace("/lol-honor-v2/v1/ballot", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolHonorV2Ballot ballot = data;
			lol::LolHonorV2ApiHonorPlayerServerRequest honorRequest;
			voli::print(c.lolUsername, "Finished game.");
			auto randomPlayer = ballot.eligiblePlayers[random_number(ballot.eligiblePlayers.size() - 1)];
			honorRequest.gameId = ballot.gameId;
			honorRequest.summonerId = randomPlayer.summonerId;
			honorRequest.honorCategory = "HEART";
			voli::print(c.lolUsername, "We are honoring: " + randomPlayer.skinName);
			lol::PostLolHonorV2V1HonorPlayer(c, honorRequest);
			auto x = lol::PostLolLobbyV2PlayAgain(c);
			if (!x.error) {
				auto lobby = lol::GetLolLobbyV2Lobby(c);
				if (lobby.data->canStartActivity == true) {
						if (lobby.data->gameConfig.queueId == 830)
							voli::print(c.lolUsername, "Joining ARAM Queue.");
						else {
							auto currentSummoner = lol::GetLolSummonerV1CurrentSummoner(c);
							if (currentSummoner->summonerLevel < 6)
								voli::print(c.lolUsername, "Joining COOP Queue.");
						}
					lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
				}
			}
			break;
		}
	});

	/**
	* Auto Accepts the Ready Check and says outputs if someone has declined.
	*/
	manager.onevent.emplace("/lol-matchmaking/v1/ready-check", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolMatchmakingMatchmakingReadyCheckResource readyCheckResource = data;
			switch (readyCheckResource.state) {
			case lol::LolMatchmakingMatchmakingReadyCheckState::StrangerNotReady_e:
				voli::print(c.lolUsername, "Someone has declined.");
				break;
			case lol::LolMatchmakingMatchmakingReadyCheckState::InProgress_e:
				if (readyCheckResource.playerResponse == lol::LolMatchmakingMatchmakingReadyCheckResponse::None_e)
					voli::print(c.lolUsername, "Ready Check accepted.");
					auto res = lol::PostLolMatchmakingV1ReadyCheckAccept(c);
				break;
			}
			break;
		}
	});

	// /lol-matchmaking/v1/search
	manager.onevent.emplace("/lol-matchmaking/v1/search", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolMatchmakingMatchmakingSearchResource searchResource = data;
			for (auto const& error : searchResource.errors) {
				if (error.errorType == "LEAVER_BUSTER_TAINTED_WARNING") {
					voli::print(c.lolUsername, "Removing Leaver Busted Warning.");
					lol::DeleteLolLeaverBusterV1NotificationsById(c, 1);
					lol::DeleteLolLeaverBusterV1NotificationsById(c, 2);
					lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
				}
			}
			switch (searchResource.searchState) {
			case lol::LolMatchmakingMatchmakingSearchState::Found_e:
				c.trashbin.erase("searchQueue");
				break;
			case lol::LolMatchmakingMatchmakingSearchState::Searching_e:
				const auto& lastQueuePos = c.trashbin.find("searchQueue");
				if (lastQueuePos != c.trashbin.end())
					break;
				c.trashbin["searchQueue"] = searchResource;
				voli::print(c.lolUsername, "In Queue (Estimated Queue Time: " + to_string((int)searchResource.estimatedQueueTime) + "s)");
				break;
			}
		}
	});

	// /lol-login/v1/session
	manager.onevent.emplace("/lol-login/v1/session", [](voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
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
					voli::print(c.lolUsername, "Logging in. (Queue Position: " + to_string(loginSession.queueStatus->estimatedPositionInQueue) + " - " + *lol::to_string(loginSession.queueStatus->approximateWaitTimeSeconds) + "s)");
				}
				break;
			}
			case lol::LolLoginLoginSessionStates::SUCCEEDED_e:
				if (loginSession.connected) {
					voli::print(c.lolUsername, "Successfully logged in.");
					if (loginSession.isNewPlayer) {
						lol::LolSummonerSummonerRequestedName name;
						name.name = capitalize(c.lolUsername);
						auto res = lol::PostLolSummonerV1Summoners(c, name);
						//TODO: Check availability and then randomize if not available
						if (res)
							voli::print(c.lolUsername, "Summoner: " + name.name + " has been created.");
						else {
							name.name = name.name + to_string(random_number(1));
							lol::PostLolSummonerV1Summoners(c, name);
						}
						lol::LolSummonerSummonerIcon icon;
						icon.profileIconId = random_number(28);
						lol::PutLolSummonerV1CurrentSummonerIcon(c, icon);
						lol::PostLolLoginV1NewPlayerFlowCompleted(c);
					}
					auto &lobbyConfig = lol::LolLobbyLobbyChangeGameDto();
					auto currentSummoner = lol::GetLolSummonerV1CurrentSummoner(c);
					if (currentSummoner->summonerLevel < 6)
						lobbyConfig.queueId = 830;
					else
						lobbyConfig.queueId = 450;
					auto res = lol::PostLolLobbyV2Lobby(c, lobbyConfig);
					if (res) {
						if (res.data->canStartActivity == true) {
							if (lobbyConfig.queueId == 450)
								voli::print(c.lolUsername, "Joining ARAM Queue.");
							else if (lobbyConfig.queueId == 830)
								voli::print(c.lolUsername, "Joining COOP Queue.");
							lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
						}
					}
				}
				break;
			case lol::LolLoginLoginSessionStates::LOGGING_OUT_e:
				voli::print(c.lolUsername, "Logging out.");
				c.wss.stop();
				break;
			}
			break;
		}
	});

	// TODO: Web Server handling
	server.addHandler("test", [&server](json data) {
		server.broadcast(ExampleEvent{ "haha" });
		return std::string("success");
	});
	service->run();
}
