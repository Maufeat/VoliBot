#pragma once
#include <SimpleWeb/server_ws.hpp>
#include <asio.hpp>
#include <json.hpp>
#include <memory>

namespace voli {
  using json = nlohmann::json;
  using IoService = asio::io_service;
  using IoServicePtr = std::shared_ptr<IoService>;
}
