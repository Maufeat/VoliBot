#include <iostream>
#include "InstanceManager.hpp"
#include "VoliServer.hpp"

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
  auto service = make_shared<IoService>();
  InstanceManager manager(service);
  VoliServer server(service, 8000);
  server.addHandler("test", [&server](json data) {
    server.broadcast(ExampleEvent{"haha"});
    return std::string("success");
  });
  service->run();
}
