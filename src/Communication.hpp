#pragma once
#include "../LeagueAPI/include/lol/def/LolStoreWallet.hpp"
#include "../LeagueAPI/include/lol/def/LolSummonerSummoner.hpp"
#include "StaticEnums.h"
#include "common.hpp"

namespace Communication
{
  using json = voli::json;
  using Status = StaticEnums::Status;

  namespace Models
  {
    struct Settings
    {
      int queue;
      bool active;
      int targetLevel;
      int targetBE;
    };
    inline void to_json(json& j, const Settings& v)
    {
      j["queue"] = v.queue;
      j["active"] = v.active;
      j["targetLevel"] = v.targetLevel;
      j["targetBE"] = v.targetBE;
    }
    inline void from_json(const json& j, Settings& v)
    {
      v.queue = j.at("queue").get<int>();
      v.active = j.at("active").get<bool>();
      v.targetLevel = j.at("targetLevel").get<int>();
      v.targetBE = j.at("targetBE").get<int>();
    }

    struct Account
    {
      int accountId = -1;
      std::string username = "";
      std::string password = "";
      std::string region = "";
      std::optional<Settings> settings = std::nullopt;
      Status status = Status::None;
      std::optional<lol::LolSummonerSummoner> summoner = std::nullopt;
      std::optional<lol::LolStoreWallet> wallet = std::nullopt;

      Account() = default;
      explicit Account(const voli::Account& acc)
      {
        accountId = acc.id;
        username = acc.username;
        password = acc.password;
        region = acc.region;
        settings = Settings();
        settings->active = acc.status;
        settings->queue = acc.queueId;
        settings->targetBE = acc.maxbe;
        settings->targetLevel = acc.maxlvl;
        status = Status::None; // TODO: Set status

        summoner = std::nullopt;
        wallet = std::nullopt;
      }
    };
    inline void to_json(json& j, const Account& v)
    {
      j["accountId"] = v.accountId;
      j["username"] = v.username;
      j["password"] = v.password;
      j["region"] = v.region;
      j["settings"] = v.settings;
      j["status"] = v.status;
      j["summoner"] = v.summoner;
      j["wallet"] = v.wallet;
    }
    inline void from_json(const json& j, Account& v)
    {
      v.accountId = j.at("accountId").get<int>();
      v.username = j.at("username").get<std::string>();
      v.password = j.at("password").get<std::string>();
      v.region = j.at("region").get<std::string>();
      v.settings = j.at("settings").get<Settings>();
      v.status = j.at("status").get<Status>();
      v.summoner = j.at("summoner").get<lol::LolSummonerSummoner>();
      v.wallet = j.at("wallet").get<lol::LolStoreWallet>();
    }
  } // namespace Models

  namespace Events
  {
    struct UpdatedDefaultSettings
    {
      static constexpr const auto NAME = "UpdatedDefaultSettings";
      Models::Settings settings;
    };
    static void to_json(json& j, const UpdatedDefaultSettings& v)
    {
      j = v.settings;
    }

    struct CreatedAccount
    {
      static constexpr const auto NAME = "CreatedAccount";
      Models::Account account;
    };
    static void to_json(json& j, const CreatedAccount& v)
    {
      j = v.account;
    }

    struct DeletedAccount
    {
      static constexpr const auto NAME = "DeletedAccount";
      int accountId;
    };
    static void to_json(json& j, const DeletedAccount& v)
    {
      j = v.accountId;
    }

    struct UpdatedAccount
    {
      static constexpr const auto NAME = "UpdatedAccount";
      Models::Account account;
    };
    static void to_json(json& j, const UpdatedAccount& v)
    {
      j = v.account;
    }

    struct AccountsList
    {
      static constexpr const auto NAME = "AccountsList";
      std::vector<Models::Account> accounts;
    };
    static void to_json(json& j, const AccountsList& v)
    {
      j = v.accounts;
    }
  } // namespace Events

} // namespace Communication
