#pragma once

#include <memory>

namespace dh
{
namespace proto
{
class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Connection;
typedef std::shared_ptr<Connection> ConnectionPtr;
}
}
