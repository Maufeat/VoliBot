#include <iostream>
#include "InstanceManager.hpp"
#include "VoliServer.hpp"
#include <lol/op/AsyncStatus.hpp>
#include <lol/op/PostLolLoginV1Session.hpp>
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
	manager.onwelcome = [](lol::LeagueClient &c) {
		std::cout << c.id << " | On Welcome request recieved" << std::endl;
	};
	///lol-login/v1/session
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
				const auto& lastQueuePos = c.trashbin.find("queuePos");
				if (lastQueuePos != c.trashbin.end())
					if (lastQueuePos->second == loginSession.queueStatus->estimatedPositionInQueue)
						break;
				c.trashbin.emplace("queuePos", loginSession.queueStatus->estimatedPositionInQueue);
				std::cout << "Logging in. (Queue Position: " << to_string(loginSession.queueStatus->estimatedPositionInQueue) << " - " << *lol::to_string(loginSession.queueStatus->approximateWaitTimeSeconds) << "s)" << std::endl;
				break;
			}
			case lol::LolLoginLoginSessionStates::SUCCEEDED_e:
				if (loginSession.connected) {
					std::cout << "Successfully logged in." << std::endl;
					//std::cout <<
				}
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
