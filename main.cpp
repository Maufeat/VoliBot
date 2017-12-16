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
	map<int, std::shared_ptr<lol::LeagueClient>> clients;

	WsServer server;
	server.config.port = 20000;
	auto &wserver = server.endpoint["^/volibot/?$"];
	wserver.on_message = [&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
		auto j = nlohmann::json::parse(message->string()).get<nlohmann::json>();
		string requestName = j["RequestName"].get<std::string>();

		cout << requestName << endl;
	};
	thread server_thread([&server]() {
		server.start();
	});
	
	clients[1] = make_shared<lol::LeagueClient>("127.0.0.1", 50647, "CU6F8Wed3kBX1kWR4Xqatw");
	clients[1]->wss.on_error = [&clients](auto c, const SimpleWeb::error_code &e) {
		if (e.value() == 10061)
		{
			cout << "Waiting for LeagueClient to be started." << endl;
			//ci.wss.start();
		}
		else
			cout << "Client: Error: " + to_string(e.value()) + " | error message: " + e.message() << endl;
	};
	clients[1]->onevent = [](auto ci, auto message) {
        cout << "From url: " << message.uri << endl;
    };
	clients[1]->onwelcome = [](auto ci) {
        cout  << "On welcome!" << endl;
    };
	clients[1]->wss.start();
    
    cin.get();
}
