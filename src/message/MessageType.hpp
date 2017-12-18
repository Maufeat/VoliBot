#pragma once
#include <inttypes.h>
namespace voli {
  enum MessageType : uint32_t {
    REQUEST = 10,  // interface to server [10, "RandomID" ,"RequestName", {requestdata}]
    RESPONSE_SUCCESS = 20, // srv 2 intfc [20, "RandomID", {resultdata}]
    RESPONSE_ERROR = 30, // srv 2 intfc [30, "RandomID", "error message"]
    EVENT = 40, // srv 2 intfc [40, "EventName", {eventData}] ,
    MESSAGE_ERROR = 50, // srv 2 intf [50, "original message as string", "error message"]
  };
}
