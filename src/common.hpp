#pragma once
#include <SimpleWeb/server_ws.hpp>
#include <asio.hpp>
#include <json.hpp>
#include <lol/base_op.hpp>
#include <memory>
#include <random>

#include <cstdlib>
#include <iomanip>
#include <rang.hpp>

#if defined(__unix__) || defined(__unix) || defined(__linux__)
#define OS_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define OS_WIN
#elif defined(__APPLE__) || defined(__MACH__)
#define OS_MAC
#else
#error Unknown Platform
#endif

#if defined(OS_LINUX) || defined(OS_MAC)
#include <cstring>
#include <unistd.h>
#elif defined(OS_WIN)
#include <VersionHelpers.h>
#include <io.h>
#include <windows.h>
#endif

#include "../sqlite/sqlite_orm.h"
#include "INIReader.h"
#include "StaticEnums.h"

using namespace sqlite_orm;

namespace voli
{
  using json = nlohmann::json;
  using IoService = asio::io_service;
  using IoServicePtr = std::shared_ptr<IoService>;

  class Config
  {
  public:
    std::string ConfigPath;
    std::string LeaguePath;
    int DefaultTargetLevel;
    int DefaultTargetBE;
    bool AutoRunOnStart;
    bool Headless;
    int MaxSessions;
    int CurSessions;
    bool DefaultActive;
    int DefaultQueue;

    explicit Config(const std::string path)
    {
      ConfigPath = path;
      INIReader reader(path);

      LeaguePath = reader.Get("General", "LeaguePath", "C:/Riot Games/League of Legends");
      AutoRunOnStart = reader.GetBoolean("General", "AutoRunOnStart", false);
      DefaultTargetLevel = reader.GetInteger("DefaultValues", "TargetLevel", 30);
      DefaultTargetBE = reader.GetInteger("DefaultValues", "TargetBE", 30000);
      DefaultActive = reader.GetBoolean("DefaultValues", "ActiveOnAdd", false);
      DefaultQueue = reader.GetInteger(
          "DefaultValues", "Queue", static_cast<int>(StaticEnums::Queue::SR_COOPvsAI_Intro));
      Headless = reader.GetBoolean("Debug", "Headless", true);

      MaxSessions = 0;
      CurSessions = 0;
    };
  };

  static Config* settings;

  struct Account
  {
    uint32_t id;
    std::string username;
    std::string password;
    std::string region;
    int queueId;
    int maxlvl;
    int maxbe;
    int status;
  };

  static std::vector<Account> accounts;

  static inline auto initStorage(const std::string& path)
  {
    using namespace sqlite_orm;
    return make_storage(path,
                        make_table("Accounts",
                                   make_column("Id", &Account::id, primary_key()),
                                   make_column("Username", &Account::username),
                                   make_column("Password", &Account::password),
                                   make_column("Region", &Account::region),
                                   make_column("QueueId", &Account::region),
                                   make_column("MaxLevel", &Account::maxlvl),
                                   make_column("MaxBE", &Account::maxbe),
                                   make_column("Status", &Account::status)));
  }

  typedef decltype(initStorage("")) Storage;
  static std::shared_ptr<Storage> database;

  class LeagueInstance
  {
  public:
    voli::Account account;
    std::string currentStatus;
    std::string auth;
    lol::WssClient wss;
    lol::HttpsClient https;
    lol::HttpsClient httpsa;
    uint32_t id;
    nlohmann::json trashbin;

    LeagueInstance(const LeagueInstance&) = delete;
    LeagueInstance(const std::string& address, const int port, const std::string& password,
                   const uint32_t id = 0) :
        auth("Basic " + SimpleWeb::Crypto::Base64::encode("riot:" + password)),
        wss(address + ":" + std::to_string(port), false),
        https(address + ":" + std::to_string(port), false),
        httpsa(address + ":" + std::to_string(port), false), id(id)
    {
      wss.config.header = {{"authorization", auth}, {"sec-websocket-protocol", "wamp"}};
    }
  };

  static std::mutex m;

  static void print_header()
  {
    std::string line_1 = "Welcome to VoliBot";
    std::string line_2 = "Version: BETA ALPHA GAMMA LAZOOOOR";
    std::string line_3 = "www.VoliBot.com";

#if defined(OS_LINUX) || defined(OS_MAC)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const int columns = w.ws_col;
#elif defined(OS_WIN)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    std::string title = "VoliBot - Current Session: 0";
    SetConsoleTitle(title.c_str());
    const int columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif

    std::cout << rang::fgB::cyan;
    for (int i = 0; i < columns; i++) {
      std::cout << "=";
    }
    std::cout << rang::fgB::yellow << std::endl;

    std::cout << std::setw(0) << std::setw((columns / 2) + line_1.size() / 2) << line_1.c_str()
              << std::endl;
    std::cout << rang::fg::white;
    std::cout << std::setw(0) << std::setw((columns / 2) + line_2.size() / 2) << line_2.c_str()
              << std::endl;
    std::cout << rang::fgB::white;
    std::cout << std::setw(0) << std::setw((columns / 2) + line_3.size() / 2) << line_3.c_str()
              << std::endl;
    std::cout << std::setw(0) << std::endl;

    std::cout << rang::fgB::cyan;
    for (int i = 0; i < columns; i++) {
      std::cout << "=";
    }
    std::cout << rang::style::reset;
  }

  // Capitalize std::string
  static std::string capitalize(std::string& s)
  {
    bool cap = true;

    for (unsigned int i = 0; i <= s.length(); i++) {
      if (isalpha(s[i]) && cap == true) {
        s[i] = toupper(s[i]);
        cap = false;
      } else if (isspace(s[i])) {
        cap = true;
      }
    }

    return s;
  }

  // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
  static std::string currentDateTime()
  {
    time_t now = time(nullptr);
    char buf[80];
    struct tm tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%d.%m.%Y %X", &tstruct);

    return buf;
  }

  static int random_number(const int size)
  {
    std::random_device rd;  // only used once to initialise (seed) engine
    std::mt19937 rng(rd()); // random-number engine used (Mersenne-Twister in this case)
    const std::uniform_int_distribution<int> uni(0, size); // guaranteed unbiased

    return uni(rng);
  }

  static void print(const std::string extra, const std::string msg)
  {
    std::lock_guard<std::mutex> mylock(m);
    std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::yellow
              << "[" + extra + "] " << rang::fgB::white << msg << rang::style::reset << std::endl;
  }

  static void printSystem(const std::string msg)
  {
    std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::white << msg
              << rang::style::reset << std::endl;
  }

  static void printError(const std::string extra, const std::string msg)
  {
    std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::yellow
              << "[" + extra + "] " << rang::fgB::red << msg << rang::style::reset << std::endl;
  }
} // namespace voli
