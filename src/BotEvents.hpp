#pragma once

#include "common.hpp"

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
#include <lol/op/PostLolHonorV2V1RewardGrantedAck.hpp>
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

#include "InstanceManager.hpp"
#include "VoliServer.hpp"

namespace voli {

	static voli::VoliServer *server;
	static voli::InstanceManager *manager;

	static void checkUpdate(voli::LeagueInstance& c) {
		auto state = lol::GetPatcherV1ProductsByProductIdState(c, "league_of_legends");
		if (state->isCorrupted) {
			voli::printSystem("League of Legends is corrupt.");
		}
		if (!state->isUpToDate) {
			// check if updateavailable
			if (state->isUpdateAvailable) {
				voli::printSystem("New Patch available. Please wait for it to download and apply.");
				lol::PostPatcherV1ProductsByProductIdStartPatchingRequest(c, "league_of_legends");
				while (!state->isUpToDate) {
					state = lol::GetPatcherV1ProductsByProductIdState(c, "league_of_legends");
					voli::printSystem("Patched: " + std::to_string(state->percentPatched) + "%");
				}
			}
			else {
				voli::printSystem("League of Legends is outdated, but no patch available...");
			}
		}
	}

	static void checkRegion(voli::LeagueInstance& c) {
		auto payload = lol::GetRiotclientGetRegionLocale(c);
		if (payload) {
			if (c.account.region != payload.data->region) {
				voli::print(c.account.username, "Region change from " + payload.data->region + " to " + c.account.region + " requested.");
				lol::PostRiotclientSetRegionLocale(c, c.account.region, payload.data->locale);
			}
		}
	}

	struct ListInstance {
		static constexpr const auto NAME = "ListInstance";
		json message;
	};

	static void to_json(json& j, const ListInstance& v) {
		j["List"] = v.message;
	}

	struct UpdateStatus {
		static constexpr const auto NAME = "UpdateStatus";
		uint32_t id;
		std::string message;
		json summoner;
		json wallet;
		json settings;
	};

	static void to_json(json& j, const UpdateStatus& v) {
		j["id"] = v.id;
		j["status"] = v.message;
		j["summoner"] = v.summoner;
		j["wallet"] = v.wallet;
		j["settings"] = v.settings;
	}


	struct UpdatePhase {
		static constexpr const auto NAME = "UpdatePhase";
		uint32_t id;
		json phaseData;
	};

	static void to_json(json& j, const UpdatePhase& v) {
		j["id"] = v.id;
		j["phaseData"] = v.phaseData;
	}

	struct LoggingOut {
		static constexpr const auto NAME = "LoggingOut";
		uint32_t id;
	};

	static void to_json(json& j, const LoggingOut& v) {
		j["id"] = v.id;
	}

	struct Settings {
		static constexpr const auto NAME = "Settings";
		uint32_t id;
		json settings;
	};

	static void to_json(json& j, const Settings& v) {
		j["id"] = v.id;
		j["settings"] = v.settings;
	}

	static void notifyUpdateStatus(std::string status, voli::LeagueInstance& client, VoliServer& server) {
		client.currentStatus = status;
		auto x = lol::GetLolSummonerV1CurrentSummoner(client);
		auto y = lol::GetLolLoginV1Wallet(client);
		json settings = { { "autoPlay", client.account.status },{ "queue", client.account.queueId } };
		server.broadcast(UpdateStatus{ client.id, status, x.data, y.data, settings });
	}

	static void notifyUpdatePhase(json phase, voli::LeagueInstance& client, VoliServer &server) {
		server.broadcast(UpdatePhase{ client.id, phase });
	}

	static void switchAccount(voli::LeagueInstance& client) {
		// Set status to finished;
		client.account.status = 2;
		// Update entry in database;
		voli::database->update(client.account);
		// Update Accounts vector;
		for (auto& account : voli::accounts) {
			if (account.id == client.id) {
				account.status = 2;
				break;
			}
		}
		// Logout request;
		lol::PostProcessControlV1ProcessQuit(client);
		// Update 
	}

	static void OnDebugPrint(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		voli::print(c.account.username, m.str());
		notifyUpdateStatus(m.str(), c, *voli::server);
	}

	/**
	* This says that our client is fully initialized and we can start
	* to send our login request. Here we have to handle wrong credentials
	* and probably ban messages and then removing the client from the
	* InstanceManager.
	*/
	static void OnStatus(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::PluginManagerResource& data) {
		if (data.state == lol::PluginManagerState::PluginsInitialized_e) {
			checkRegion(c);
			checkUpdate(c);
			lol::LolLoginUsernameAndPassword uap;
			uap.password = c.account.password;
			uap.username = c.account.username;
			auto res = lol::PostLolLoginV1Session(c, uap);
			if (res) {
				if (res.data->error) {
					if (res.data->error->messageId == "INVALID_CREDENTIALS") {
						voli::print(c.account.username, "Wrong credentials.");
						c.wss.stop();
					}
				}
			}
		}
	}

	static void OnGatekeeperPenalty(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const std::vector<lol::LolMatchmakingMatchmakingSearchErrorResource>& data) {
		for (auto const& error : data) {
			if (error.penaltyTimeRemaining <= 1 && data.size() == 1) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				voli::print(c.account.username, "Joining queue.");
				lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
			}
			//voli::print(c.lolUsername, error.message + " | " + to_string(error.penaltyTimeRemaining) + " | " + to_string(t));
		}
	}

	static void OnPreEndOfGameSequence(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::LolPreEndOfGameSequenceEvent& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::PostLolPreEndOfGameV1CompleteBySequenceEventName(c, data.name);
			break;
		}
	}

	/**
	* Saves our current summoner into trashbin. We can and should probably
	* just request this endpoint when needed instead of storing it. But
	* this has to be tested if this here fails, if not we can leave this.
	*/
	static void OnCurrentSummoner(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::LolSummonerSummoner& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			c.trashbin["currentSummoner"] = data;
			break;
		}
	}

	static void OnLoginWallet(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const lol::LolLoginLoginSessionWallet& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			c.trashbin["wallet"] = data;
			break;
		}
	}

	static void OnHonorGranted(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		lol::PostLolHonorV2V1RewardGrantedAck(c);
	}

	static void OnGameFlowSession(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		if (data.at("phase").get<std::string>() == "None")
			return;
		switch (t) {
		case lol::PluginResourceEventType::Update_e:
		case lol::PluginResourceEventType::Create_e:
			lol::LolGameflowGameflowSession flow = data;
			notifyUpdatePhase(flow, c, *voli::server);
			switch (flow.phase) {
			case lol::LolGameflowGameflowPhase::None_e:
				break;
			case lol::LolGameflowGameflowPhase::WaitingForStats_e:
			{
				notifyUpdateStatus("Waiting for Stats", c, *voli::server);
				voli::print(c.account.username, "Waiting for Stats. (End of Game)");
				lol::PostLolGameflowV1PreEndGameTransition(c, true);
				auto eog = lol::GetLolEndOfGameV1EogStatsBlock(c);
				if (eog.data) {
					lol::LolEndOfGameEndOfGameStats eogStats = *eog.data;
					notifyUpdateStatus("Finished game (+ " + std::to_string(eogStats.experienceEarned) + " EXP)", c, *voli::server);
					voli::print(c.account.username, "Finished game. (+ " + std::to_string(eogStats.experienceEarned) + " EXP)");
					auto playAgain = lol::PostLolLobbyV2PlayAgain(c);
					if (playAgain && c.account.status == 1) {
						auto lobby = lol::GetLolLobbyV2Lobby(c);
						if (lobby.data->canStartActivity == true) {
							if (lobby.data->gameConfig.queueId == 450)
								voli::print(c.account.username, "Joining ARAM Queue.");
							else {
								if (eogStats.currentLevel < 6)
									voli::print(c.account.username, "Joining COOP Queue.");
								else {
									auto &lobbyConfig = lol::LolLobbyLobbyChangeGameDto();
									lobbyConfig.queueId = c.account.queueId;
									auto res = lol::PostLolLobbyV2Lobby(c, lobbyConfig);
									if (res)
										if (res.data->canStartActivity == true)
											voli::print(c.account.username, "Changed now to " + std::to_string(c.account.queueId) + " QueueID.");
								}
							}
							lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
						}
					}
				}
				break;
			}
			case lol::LolGameflowGameflowPhase::GameStart_e:
				notifyUpdateStatus("Starting League of Legends.", c, *voli::server);
				voli::print(c.account.username, "Starting League of Legends.");
				break;
			case lol::LolGameflowGameflowPhase::Reconnect_e:
				notifyUpdateStatus("Reconnecting...", c, *voli::server);
				voli::print(c.account.username, "Client probably crashed. Reconnecting...");
				lol::PostLolGameflowV1Reconnect(c);
				break;
			case lol::LolGameflowGameflowPhase::FailedToLaunch_e:
				notifyUpdateStatus("Reconnecting...", c, *voli::server);
				voli::print(c.account.username, "Failed to Launch. Reconnecting...");
				lol::PostLolGameflowV1Reconnect(c);
				break;
			case lol::LolGameflowGameflowPhase::InProgress_e:
				notifyUpdateStatus("In Game", c, *voli::server);
				break;
			}
			break;
		}
	}
	
	/**
	* Handling the champion selection screen. Currently we are getting the
	* pickable champions list and choose a random champion from that list
	* which should work in every queue. As for ARAM, we check if the actions
	* size is 0, because ARAM requires no actions. Here we should also handle
	* trading requests in ARAM, to give Human Players prefered champions.
	*/
	static void OnChampSelectSession(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
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
						voli::print(c.account.username, "In Champion Selection");
						voli::print(c.account.username, "We've got: " + championInfos.data->name);
						notifyUpdateStatus("We've got: " + championInfos.data->name, c, *voli::server);
						notifyUpdateStatus("Champion Selection", c, *voli::server);
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
								voli::print(c.account.username, "In Champion Selection");
								voli::print(c.account.username, "We are picking: " + championInfos->name);
								notifyUpdateStatus("We are picking: " + championInfos.data->name, c, *voli::server);
								notifyUpdateStatus("Champion Selection", c, *voli::server);
								lol::LolChampSelectChampSelectAction csAction;
								csAction.actorCellId = champSelectSession.localPlayerCellId;
								csAction.championId = randomId;
								csAction.completed = false;
								csAction.type = "pick";
								csAction.id = aaction.at("id").get<uint32_t>();
								lol::PatchLolChampSelectV1SessionActionsById(c, csAction.id, csAction);
								lol::PostLolChampSelectV1SessionActionsByIdComplete(c, csAction.id);
							}
							else if (aaction.at("type").get<std::string>() == "ban") {
								voli::print(c.account.username, "In Champion Selection");
								voli::print(c.account.username, "We have to ban.");
							}
						}
					}
				}
			}
			break;
		}
	}
	
	/**
	* We are using the Honor System to check if the game is over. We should
	* probably try to find another way to re-join the queue. But as for now
	* this looks okay since it is getting triggered on every PVP EOG screen.
	* We also have to change the queueId here when reaching Level 6.
	*/
	static void OnHonorBallot(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
			lol::LolHonorV2Ballot ballot = data;
			lol::LolHonorV2ApiHonorPlayerServerRequest honorRequest;
			auto randomPlayer = ballot.eligiblePlayers[random_number(ballot.eligiblePlayers.size() - 1)];
			honorRequest.gameId = ballot.gameId;
			honorRequest.summonerId = randomPlayer.summonerId;
			std::string aCats[3] = { "SHOTCALLER", "HEART", "COOL" };
			int rndInt = random_number(2);
			auto rndCat = aCats[rndInt];
			honorRequest.honorCategory = rndCat;
			voli::print(c.account.username, "We are honoring: " + randomPlayer.skinName + " with " + capitalize(rndCat));
			lol::PostLolHonorV2V1HonorPlayer(c, honorRequest);
			break;
		}
	}
	
	/**
	* Auto Accepts the Ready Check and says outputs if someone has declined.
	*/
	static void OnMatchmakingReadyCheck(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			lol::LolMatchmakingMatchmakingReadyCheckResource readyCheckResource = data;
			switch (readyCheckResource.state) {
			case lol::LolMatchmakingMatchmakingReadyCheckState::StrangerNotReady_e:
				voli::print(c.account.username, "Someone has declined.");
				break;
			case lol::LolMatchmakingMatchmakingReadyCheckState::InProgress_e:
				if (readyCheckResource.playerResponse == lol::LolMatchmakingMatchmakingReadyCheckResponse::None_e)
					voli::print(c.account.username, "Ready Check accepted.");
				auto res = lol::PostLolMatchmakingV1ReadyCheckAccept(c);
				break;
			}
			break;
		}
	}

	/**
	* Here we handle Leaver Buster and output current queue times.
	* We have to figure out how to wait for a leaver busted penality
	* without creating a new thread and using std::chrono sleeps.
	*/
	static void OnMatchmakingSearch(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
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
				const auto& penalty = c.trashbin.find("penalty");
				if (lastQueuePos != c.trashbin.end())
					break;
				if (penalty != c.trashbin.end() && searchResource.lowPriorityData.penaltyTimeRemaining != 0)
					break;
				if (searchResource.lowPriorityData.penaltyTimeRemaining != 0) {
					c.trashbin["penalty"] = searchResource;
					notifyUpdateStatus("Couldn't queue. Remaining Penalty: " + std::to_string((int)searchResource.lowPriorityData.penaltyTimeRemaining) + "s", c, *voli::server);
					voli::print(c.account.username, "Couldn't queue. Remaining Penalty: " + std::to_string((int)searchResource.lowPriorityData.penaltyTimeRemaining) + "s");
					break;
				}
				else {
					c.trashbin["searchQueue"] = searchResource;
					notifyUpdateStatus("In Queue (" + std::to_string((int)searchResource.estimatedQueueTime) + "s)", c, *voli::server);
					voli::print(c.account.username, "In Queue (Estimated Queue Time: " + std::to_string((int)searchResource.estimatedQueueTime) + "s)");
				}
				break;
			}
		}
	}

	static void OnLeaverBusterNotification(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
		switch (t) {
		case lol::PluginResourceEventType::Delete_e:
			break;
		case lol::PluginResourceEventType::Create_e:
		case lol::PluginResourceEventType::Update_e:
			std::vector<lol::LeaverBusterNotificationResource> lbNot = data;
			for (auto &notif : lbNot)
				lol::DeleteLolLeaverBusterV1NotificationsById(c, notif.id);
			break;
		}
	}

	/**
	* Handles our Login. We check for login state and outputs relevant states.
	* If a new account is logged in, we create the summoner by capitalizing the
	* username, if it fails we add one random number. Additional checks have to
	* be implemented here, like check for 12 characters and other things.
	* After a successfull login, we queue up for ARAM. If the account is not
	* eligible to queue for ARAM, queue for INTRO COOP.
	*/
	static void OnLoginSession(voli::LeagueInstance& c, const std::smatch& m, lol::PluginResourceEventType t, const nlohmann::json& data) {
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
						if (lastQueuePos->at("approximateWaitTimeSeconds").get<uint64_t>() == loginSession.queueStatus->approximateWaitTimeSeconds)
							break;
					c.trashbin["queuePos"] = loginSession.queueStatus;
					notifyUpdateStatus("Logging in. (" + std::to_string(loginSession.queueStatus->estimatedPositionInQueue) + " | " + *lol::to_string(loginSession.queueStatus->approximateWaitTimeSeconds) + "s)", c, *voli::server);
					voli::print(c.account.username, "Logging in. (Queue Position: " + std::to_string(loginSession.queueStatus->estimatedPositionInQueue) + " - " + *lol::to_string(loginSession.queueStatus->approximateWaitTimeSeconds) + "s)");
				}
				break;
			}
			case lol::LolLoginLoginSessionStates::SUCCEEDED_e:
				if (loginSession.connected) {
					voli::print(c.account.username, "Successfully logged in.");
					notifyUpdateStatus("Logged in.", c, *voli::server);
					if (loginSession.isNewPlayer) {
						lol::LolSummonerSummonerRequestedName name;
						name.name = capitalize(c.account.username);
						auto res = lol::PostLolSummonerV1Summoners(c, name);
						if (!res.error)
							voli::print(c.account.username, "Summoner: " + name.name + " has been created.");
						else {
							name.name = name.name + std::to_string(random_number(1));
							lol::PostLolSummonerV1Summoners(c, name);
						}
						lol::LolSummonerSummonerIcon icon;
						icon.profileIconId = random_number(28);
						lol::PutLolSummonerV1CurrentSummonerIcon(c, icon);
						lol::PostLolLoginV1NewPlayerFlowCompleted(c);
					}
					auto &lobbyConfig = lol::LolLobbyLobbyChangeGameDto();
					auto currentSummoner = lol::GetLolSummonerV1CurrentSummoner(c);
					c.trashbin["currentSummoner"] = currentSummoner.data;
					if (currentSummoner->summonerLevel < 6)
						lobbyConfig.queueId = 830;
					else
						lobbyConfig.queueId = c.account.queueId;
					auto res = lol::PostLolLobbyV2Lobby(c, lobbyConfig);
					if (res && c.account.status == 1) {
						if (res.data->canStartActivity == true) {
							if (lobbyConfig.queueId == 450) {
								notifyUpdateStatus("Joining ARAM Queue.", c, *voli::server);
								voli::print(c.account.username, "Joining ARAM Queue.");
							}
							else if (lobbyConfig.queueId == 830) {
								notifyUpdateStatus("Joining COOP Queue.", c, *voli::server);
								voli::print(c.account.username, "Joining COOP Queue.");
							}
							else {
								notifyUpdateStatus("Joining " + std::to_string(c.account.queueId) + " Queue.", c, *voli::server);
								voli::print(c.account.username, "Joining COOP Queue.");
							}
							auto queue = lol::PostLolLobbyV2LobbyMatchmakingSearch(c);
							if (queue.error->errorCode == "GATEKEEPER_RESTRICTED") {
								voli::print(c.account.username, queue.error->message);
								for (auto const& error : queue.error->payload) {
									notifyUpdateStatus("Waiting to Queue (" + std::to_string((error.at("remainingMillis").get<int>() / 1000)) + "s)", c, *voli::server);
									voli::print(c.account.username, "Time until cleared: " + std::to_string((error.at("remainingMillis").get<int>() / 1000)) + "s");
								}
							}
						}
					}
				}
				break;
			case lol::LolLoginLoginSessionStates::LOGGING_OUT_e:
				voli::print(c.account.username, "Logging out.");
				auto y = lol::PostProcessControlV1ProcessQuit(c);
				c.wss.stop();
				break;
			}
			break;
		}
	}
	
}
