#pragma once
#include <SimpleWeb/server_ws.hpp>
#include <lol/base_op.hpp>
#include <asio.hpp>
#include <json.hpp>
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
#include <unistd.h>
#include <cstring>
#elif defined(OS_WIN)
#include <windows.h>
#include <io.h>
#include <VersionHelpers.h>
#endif

namespace voli {
	using json = nlohmann::json;
	using IoService = asio::io_service;
	using IoServicePtr = std::shared_ptr<IoService>;



	class LeagueInstance {
	public:
		std::string lolUsername;
		std::string lolPassword;
		std::string lolRegion;
		std::string currentStatus;
        std::string auth;
        lol::WssClient wss;
        lol::HttpsClient https;
        lol::HttpsClient httpsa;
        uint32_t id;
        json trashbin;

        LeagueInstance(const LeagueInstance&) = delete;
        LeagueInstance(const std::string& address, int port, const std::string& password, uint32_t id = 0) :
            auth("Basic " + SimpleWeb::Crypto::Base64::encode("riot:" + password)),
            wss(address + ":" + std::to_string(port), false),
            https(address + ":" + std::to_string(port), false),
            httpsa(address + ":" + std::to_string(port), false)
        {
            wss.config.header = {
                { "authorization", auth },
                { "sec-websocket-protocol", "wamp" }
            };
        }
	};


	static std::mutex m;

	static void print_header() {

		std::string line_1 = "Welcome to VoliBot";
		std::string line_2 = "Version: BETA ALPHA GAMMA LAZOOOOR";
		std::string line_3 = "www.VoliBot.com";

		int columns;

#if defined(OS_LINUX) || defined(OS_MAC)
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		columns = w.ws_col;
#elif defined(OS_WIN)
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		SetConsoleTitle("VoliBot - Title");
		columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif

		std::cout << rang::fgB::cyan;
		for (int i = 0; i < columns; i++) {
			std::cout << "=";
		}
		std::cout << rang::fgB::yellow << std::endl;

		std::cout << std::setw(0) << std::setw((columns / 2) + line_1.size() / 2) << line_1.c_str() << std::endl;
		std::cout << rang::fg::white;
		std::cout << std::setw(0) << std::setw((columns / 2) + line_2.size() / 2) << line_2.c_str() << std::endl;
		std::cout << rang::fgB::white;
		std::cout << std::setw(0) << std::setw((columns / 2) + line_3.size() / 2) << line_3.c_str() << std::endl;
		std::cout << std::setw(0) << std::endl;

		std::cout << rang::fgB::cyan;
		for (int i = 0; i < columns; i++) {
			std::cout << "=";
		}
		std::cout << rang::style::reset;

	}

	// Capitalize std::string
	static std::string capitalize(std::string &s)
	{
		bool cap = true;

		for (unsigned int i = 0; i <= s.length(); i++)
		{
			if (isalpha(s[i]) && cap == true)
			{
				s[i] = toupper(s[i]);
				cap = false;
			}
			else if (isspace(s[i]))
			{
				cap = true;
			}
		}

		return s;
	}

	// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
	static const std::string currentDateTime() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%d.%m.%Y %X", &tstruct);

		return buf;
	}

	static int random_number(int size) {
		std::random_device rd;     // only used once to initialise (seed) engine
		std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
		std::uniform_int_distribution<int> uni(0, size); // guaranteed unbiased

		return uni(rng);
	}

	static void print(std::string extra, std::string msg) {
		std::lock_guard<std::mutex> mylock(m);
		std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::yellow << "[" + extra + "] " << rang::fgB::white << msg << rang::style::reset << std::endl;
	}

	static void printSystem(std::string msg) {
		std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::white << msg << rang::style::reset << std::endl;
	}

	static void printError(std::string extra, std::string msg) {
		std::cout << rang::fgB::cyan << "[" + currentDateTime() + "] " << rang::fgB::yellow << "[" + extra + "] " << rang::fgB::red << msg << rang::style::reset << std::endl;
	}
}
