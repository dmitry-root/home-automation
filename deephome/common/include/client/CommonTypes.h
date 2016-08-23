#pragma once

#include <limits>
#include <cstdint>
#include <vector>


namespace dh
{
namespace client
{

class DeviceCommand;
class Filter;

class Dispatcher;

static const uint32_t infinite_timeout = std::numeric_limits<uint32_t>::max();

typedef std::vector<uint8_t> Body;

}
}
