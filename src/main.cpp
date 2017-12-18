#include <iostream>
#include "InstanceManager.hpp"
#include "VoliServer.hpp"

using namespace std;
using namespace voli;

int main()
{
  auto service = make_shared<IoService>();
  InstanceManager manager(service);
  VoliServer server(service, 8000);
  server.addHandler("xd", [](const std::string& arg) -> std::string {
    return "arg was " + arg;
  });
  service->run();
}
