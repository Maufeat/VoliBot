#include "BotEvents.hpp"
#include "InstanceManager.hpp"
#include "VoliServer.hpp"
#include <algorithm>
#include <iostream>

#include "Communication.hpp"

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
      case 0: cIdle++; break;
      case 1:
        cRunning++;
        if (voli::settings->AutoRunOnStart)
          voli::manager->Start(account);
        break;
      case 2: cFinished++; break;
    }
  }

  voli::printSystem("Accounts loaded: " + to_string(voli::accounts.size()) +
                    " | Idle: " + to_string(cIdle) + " - Running: " + to_string(cRunning) +
                    " - Finished: " + to_string(cFinished));
  if (cRunning > 0 && voli::settings->AutoRunOnStart)
    voli::printSystem("Auto starting all " + to_string(cRunning) +
                      " Accounts with \"Running\"-State.");

  voli::manager->onevent = {
      // { ".*", voli::OnDebugPrint },
      {"/plugin-manager/v1/status", voli::OnStatus},
      {"/lol-matchmaking/v1/search/errors", voli::OnGatekeeperPenalty},
      {"/lol-pre-end-of-game/v1/currentSequenceEvent", voli::OnPreEndOfGameSequence},
      {"/lol-summoner/v1/current-summoner", voli::OnCurrentSummoner},
      {"/lol-login/v1/wallet", voli::OnLoginWallet},
      {"/lol-honor-v2/v1/reward-granted", voli::OnHonorGranted},
      {"/lol-gameflow/v1/session", voli::OnGameFlowSession},
      {"/lol-champ-select/v1/session", voli::OnChampSelectSession},
      {"/lol-honor-v2/v1/ballot", voli::OnHonorBallot},
      {"/lol-matchmaking/v1/ready-check", voli::OnMatchmakingReadyCheck},
      {"/lol-matchmaking/v1/search", voli::OnMatchmakingSearch},
      {"/lol-leaver-buster/v1/notifications", voli::OnLeaverBusterNotification},
      {"/lol-login/v1/session", voli::OnLoginSession}};

#pragma region Core x UI API - v2
  server->addHandler("GetAccountList", [](json data) {
    auto output = std::vector<Communication::Models::Account>();

    for (auto const& lol : manager->GetAll()) {
      auto account = Communication::Models::Account(lol.second->account);
      account.status = StaticEnums::Status::None; // TODO: Add status
      account.summoner = lol.second->trashbin["currentSummoner"];
      account.wallet = lol.second->trashbin["wallet"];

      output.push_back(account);
    }

    for (auto const& lol : accounts) {
      if (std::find_if(
              output.begin(), output.end(), [&lol](const Communication::Models::Account& s) {
                return s.accountId == lol.id;
              }) != output.end())
        continue;

      output.push_back(Communication::Models::Account(lol));
    }

    return json(output);
  });

  server->addHandler("GetDefaultSettings", [](json data) {
    auto settings = Communication::Models::Settings();

    settings.active = voli::settings->DefaultActive;
    settings.targetBE = voli::settings->DefaultTargetBE;
    settings.targetLevel = voli::settings->DefaultTargetLevel;
    settings.queue = voli::settings->DefaultQueue;

    return json(settings);
  });

  server->addHandler("GetAccountDetails", [](json data) {
    std::optional<Communication::Models::Account> output;
    auto account_id = data.get<int>();

    for (auto const& lol : manager->GetAll()) {
      if (lol.second->account.id != account_id)
        continue;

      output = Communication::Models::Account(lol.second->account);

      output->status = StaticEnums::Status::None; // TODO: Add status
      output->summoner = lol.second->trashbin["currentSummoner"];
      output->wallet = lol.second->trashbin["wallet"];
    }

    if (!output.has_value()) {
      for (auto const& lol : accounts) {
        if (lol.id != account_id)
          continue;

        output = Communication::Models::Account(lol);
      }
    }

    // TODO: Make an "error" class? Or just return empty like we do now.
    return json(output);
  });

  server->addHandler("DeleteAccount", [](json data) {
    auto id = data.get<int>();
    auto db_user = voli::database->get_no_throw<voli::Account>(id);

    if (!db_user)
      return false;

    voli::printSystem("Removing Account (" + db_user->username + ")");
    voli::database->remove<voli::Account>(id);
    voli::accounts.erase(
        std::remove_if(voli::accounts.begin(),
                       voli::accounts.end(),
                       [&](voli::Account const& acc) { return acc.id == db_user->id; }),
        voli::accounts.end());

    voli::server->broadcast(Communication::Events::DeletedAccount{id});

    auto instance = voli::manager->Get(id);
    if (!instance)
      return true;

    if (!lol::PostProcessControlV1ProcessQuit(*instance))
      return false;

    voli::print(instance->account.username, "Logging out.");

    voli::manager->Remove(id);
    return true;
  });

  voli::server->addHandler("CreateAccount", [](json data) {
    auto account = data.get<Communication::Models::Account>();

    voli::Account user{
        -1,
        account.username,
        account.password,
        account.region,
        account.settings->queue ? account.settings->queue : voli::settings->DefaultQueue,
        account.settings->targetLevel ? account.settings->targetLevel
                                      : voli::settings->DefaultTargetLevel,
        account.settings->targetBE ? account.settings->targetBE : voli::settings->DefaultTargetBE,
        account.settings->active ? account.settings->active : voli::settings->DefaultActive};

    // TODO: Check if the account already exists?

    // Add to the DB and the account list
    auto inserted_id = voli::database->insert(user);
    voli::accounts.push_back(user);

    // Assign an ID to the account to be able to refer to it later
    user.id = inserted_id;

    // Print and broadcast output
    voli::printSystem("Adding new Account (" + user.username + ") at " + user.region);
    voli::server->broadcast(
        Communication::Events::CreatedAccount{Communication::Models::Account(user)});

    // Return
    return json(Communication::Models::Account(user));
  });

  voli::server->addHandler("UpdateAccountSettings", [](json data) {
    auto id = data.at("accountId").get<int>();
    auto settings = data.at("Settings").get<Communication::Models::Settings>();

    std::optional<Communication::Models::Account> output = std::nullopt;
    for (auto lol : accounts) {
      if (lol.id != id)
        continue;

      lol.queueId = settings.queue ? settings.queue : lol.queueId;
      lol.maxlvl = settings.targetLevel ? settings.targetLevel : lol.maxlvl;
      lol.maxbe = settings.targetBE ? settings.targetBE : lol.maxbe;
      lol.status = settings.active ? settings.active : lol.status;

      output = Communication::Models::Account(lol);
      break;
    }

    if (output.has_value())
      voli::server->broadcast(Communication::Events::UpdatedAccount{*output});

    return output.has_value();
  });

  voli::server->addHandler("UpdateAccount", [](json data) {
    auto id = data.at("accountId").get<int>();
    auto account = data.at("Account").get<Communication::Models::Account>();
    auto settings = account.settings;

    std::optional<Communication::Models::Account> output = std::nullopt;
    for (auto lol : accounts) {
      if (lol.id != id)
        continue;

      lol.username = account.username;
      lol.password = account.password;
      lol.region = account.region;
      lol.queueId = settings->queue ? settings->queue : lol.queueId;
      lol.maxlvl = settings->targetLevel ? settings->targetLevel : lol.maxlvl;
      lol.maxbe = settings->targetBE ? settings->targetBE : lol.maxbe;
      lol.status = settings->active ? settings->active : lol.status;

      output = Communication::Models::Account(lol);
      break;
    }

    if (output.has_value())
      voli::server->broadcast(Communication::Events::UpdatedAccount{*output});

    return output.has_value();
  });

  voli::server->addHandler("UpdateDefaultSettings", [](json data) {
    // TODO: Save the new default settings to the .ini

    auto settings = data.get<Communication::Models::Settings>();

    if (settings.queue)
      voli::settings->DefaultQueue = settings.queue;
    if (settings.targetLevel)
      voli::settings->DefaultTargetLevel = settings.targetLevel;
    if (settings.targetBE)
      voli::settings->DefaultTargetBE = settings.targetBE;
    if (settings.active)
      voli::settings->DefaultActive = settings.active;

    settings.queue = voli::settings->DefaultQueue;
    settings.active = voli::settings->DefaultActive;
    settings.targetBE = voli::settings->DefaultTargetBE;
    settings.targetLevel = voli::settings->DefaultTargetLevel;

    voli::server->broadcast(Communication::Events::UpdatedDefaultSettings{settings});
    return true;
  });
#pragma endregion

#pragma region Core x UI API - v1
  voli::server->addHandler("RequestInstanceList", [](json data) {
    // Store all Accounts in message.
    json message;
    for (auto const& lol : voli::accounts) {
      message[lol.id] = {{"id", lol.id}, {"status", "Logged out"}, {"summoner", ""}};
    }
    // Replace Accounts that are running with current states.
    for (auto const& lol : voli::manager->GetAll()) {
      json settings = {{"autoPlay", lol.second->account.status},
                       {"queue", lol.second->account.queueId},
                       {"region", lol.second->account.region},
                       {"targetLevel", lol.second->account.maxlvl},
                       {"targetBE", lol.second->account.maxbe}};
      message[lol.second->id] = {{"id", lol.second->id},
                                 {"status", lol.second->currentStatus},
                                 {"summoner", lol.second->trashbin["currentSummoner"]},
                                 {"wallet", lol.second->trashbin["wallet"]},
                                 {"settings", settings}};
    }
    voli::server->broadcast(ListInstance{message});
    return std::string("success");
  });

  voli::server->addHandler("RequestInstanceStart", [](json data) {
    std::string username = data.at("username").get<std::string>();
    std::string password = data.at("password").get<std::string>();
    std::string region = data.at("region").get<std::string>();
    int queueId = data.at("queue").get<int>();
    bool autoplay = data.at("autoplay").get<bool>();
    int targetLevel = data.at("targetlevel").get<int>();
    int targetBE = data.at("targetbe").get<int>();

    int autoPlay = (autoplay) ? 1 : 0;
    Account user{-1, username, password, region, queueId, targetLevel, targetBE, autoPlay};
    auto insertedId = voli::database->insert(user);
    user.id = insertedId;

    voli::printSystem("Adding new Account (" + user.username + ") at " + user.region);
    voli::server->broadcast(UpdateStatus{user.id, "Logged out"});

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
      voli::server->broadcast(LoggingOut{user->id});
      voli::accounts.erase(std::remove_if(voli::accounts.begin(),
                                          voli::accounts.end(),
                                          [&](Account const& acc) { return acc.id == user->id; }),
                           voli::accounts.end());
      if (x) {
        auto y = lol::PostProcessControlV1ProcessQuit(*x);
        if (y) {
          voli::print(x->account.username, "Logging out.");
          voli::manager->Remove(id);
          return std::string("success");
        }
        return std::string("failed");
      }
      return std::string("success");
    }
    return std::string("failed");
  });

  voli::server->addHandler("RequestChangeSettings", [](json data) {
    try {
      int id = data.at("id").get<int>();
      int queueId = data.at("queue").get<int>();
      bool autoplay = data.at("autoplay").get<bool>();
      int targetLevel = data.at("targetlevel").get<int>();
      int targetBE = data.at("targetbe").get<int>();

      auto x = voli::manager->Get(id);
      if (auto user = voli::database->get_no_throw<Account>(id)) {
        user->maxbe = targetBE;
        user->maxlvl = targetLevel;
        user->status = autoplay;
        user->queueId = queueId;

        voli::database->update(*user);
        // Update Accounts vector;
        for (auto& account : voli::accounts) {
          if (account.id == user->id) {
            account.maxbe = user->maxbe;
            account.maxlvl = user->maxlvl;
            account.status = user->status;
            account.queueId = user->queueId;
            break;
          }
        }
      } else {
        return std::string("failed");
      }
    } catch (...) {
      return std::string("failed");
    }

    return std::string("success");
  });
#pragma endregion Leaving this in to support the old versions of both UIs just a tiny bit more

  voli::printSystem("Access VoliBot Web Manager: http://cdn.volibot.com/bot");
  service->run();
}
