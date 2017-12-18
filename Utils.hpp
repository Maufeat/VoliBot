#pragma once
#include <string>
#include <random>
#include <fstream>
#include <iostream>
#include <asio.hpp>
#include "LeagueAPI/tiny-process-library/process.hpp";

namespace Utils {

	std::string get_exe(std::string path) {
		if (std::ifstream(path + "/LeagueClient.bat")) {
			return "\\LeagueClient.bat";
		}
		else if (std::ifstream(path + "/LeagueClient.exe")) {
			return "\\LeagueClient.exe";
		}
		else if (std::ifstream(path + "/LeagueClient.app")) {
			return "\\LeagueClient.app";
		}
		return "";
	};

	long launchWithArgs(std::string path, std::string args)
	{
		TinyProcessLib::Process lcuProcess(path + get_exe(path) + " " + args);
		auto exit_status = lcuProcess.get_exit_status();
		return lcuProcess.get_id();
	};

	uint16_t freePort()
	{
		asio::io_service service;
		asio::ip::tcp::acceptor acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
		return acceptor.local_endpoint().port();
	}

	std::string random_string(size_t length)
	{
		static auto& chrs = "0123456789"
			"abcdefghijklmnopqrstuvwxyz-"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

		thread_local static std::mt19937 rg{ std::random_device{}() };
		thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

		std::string s;

		s.reserve(length);

		while (length--)
			s += chrs[pick(rg)];

		return s;
	};


};