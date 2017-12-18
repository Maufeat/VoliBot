#include <iostream>
#include <lol/base_op.hpp>
#include "InstanceManager.hpp"
#include <SimpleWeb/server_ws.hpp>
#include "LeagueAPI/tiny-process-library/process.hpp";

using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

int main()
{

	WsServer server;
	InstanceManager im;
	server.config.port = 20000;

	map<int, std::shared_ptr<lol::LeagueClient>> clients;

	auto &wserver = server.endpoint["^/volibot/?$"];
	wserver.on_message = [&server, &im](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
		auto j = nlohmann::json::parse(message->string()).get<nlohmann::json>();
		auto send_stream = make_shared<WsServer::SendStream>();
		string requestName = j["RequestName"].get<std::string>();
		if (requestName == "RequestInstanceList") {
			nlohmann::json res;
			res["EventName"] = "EventListInstance";
			res["List"] = nlohmann::json::array({});
			for (auto const &client : im.GetAll()){
				res["List"].emplace_back(nlohmann::json::array({ { { "Id", client.first } } }));
			}
			*send_stream << res.dump();
			for (auto &a_connection : server.get_connections())
				a_connection->send(send_stream);
		}
		else if (requestName == "RequestInstanceStart") {
			thread client([&im, &server, &j]() {
				// TODO: Add Username, Password and Region here and in Web Interface
				//im.Start(49821, "55j584nSsfxGZsStdNsp_g");
				im.Start(server, j);
			});
			client.detach();
		}
	};

	im.onwelcome = [&server](int id, auto ci) {
		std::cout << "OnWelcome from Id : " << id << std::endl;
		//SEND SOMETHING TO WEBINTERFACE
	};

	im.onevent = [&server](int id, auto ci, auto x) {
		std::cout << "OnEvent from Id : " << id << std::endl;
		//SEND SOMETHING TO WEBINTERFACE
	};

	thread server_thread([&server]() {
		server.start();
	});

	server_thread.join();
	
    cin.get();
}
