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
	};
	manager.onevent.emplace(".*", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
    // auto uri = m.str();
    // voli::print("Test", uri.c_str());
	});
  manager.onevent.emplace("/plugin-manager/v1/status", [](lol::LeagueClient& c, const std::smatch& m, lol::PluginResourceEventType t, const json& data) {
    auto res = lol::PostLolLoginV1Session(c, { "b24241321", "moonshadow565" });
    if (res)
      cout << "OK:" << lol::to_string(*(res.data)) << '\n';
    else
      cout << "BAD: " << lol::to_string(*(res.error)) << '\n';
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
