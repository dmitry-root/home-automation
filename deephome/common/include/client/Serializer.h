#pragma once

#include <cstdint>
#include <string>

#include "util/Assert.h"
#include "client/CommonTypes.h"


namespace dh
{
namespace client
{


template <typename T>
class Serializer
{
public:
	static void serialize(const T&, Body&) { DH_ASSERT("not implemented"); }
	static bool deserialize(const Body&, T&) { DH_ASSERT("not implemented"); return false; }
};


template <>
class Serializer<uint8_t>
{
public:
	static void serialize(uint8_t value, Body& result)
	{
		result.assign(1, value);
	}

	static bool deserialize(const Body& data, uint8_t& result)
	{
		if (data.size() != 1)
			return false;
		result = data[0];
		return true;
	}
};


template <>
class Serializer<uint16_t>
{
public:
	static void serialize(uint16_t value, Body& result)
	{
		result.resize(2);
		result[0] = value >> 8;
		result[1] = value & 0xff;
	}

	static bool deserialize(const Body& data, uint16_t& result)
	{
		if (data.size() != 2)
			return false;
		result = (static_cast<uint16_t>(data[0]) << 8) | data[1];
		return true;
	}
};


template <>
class Serializer<uint32_t>
{
public:
	static void serialize(uint32_t value, Body& result)
	{
		result.resize(4);
		result[0] = value >> 24;
		result[1] = (value >> 16) & 0xff;
		result[2] = (value >> 8) & 0xff;
		result[3] = value & 0xff;
	}

	static bool deserialize(const Body& data, uint32_t& result)
	{
		if (data.size() != 4)
			return false;
		result =
		        (static_cast<uint32_t>(data[0]) << 24) |
		        (static_cast<uint32_t>(data[1]) << 16) |
		        (static_cast<uint32_t>(data[2]) << 8) |
		        data[3];
		return true;
	}
};


template<>
class Serializer<std::string>
{
public:
	static void serialize(const std::string& value, Body& result)
	{
		DH_VERIFY( value.length() <= 8 );
		const uint8_t* const buffer = reinterpret_cast<const uint8_t*>( value.data() );
		result.assign( buffer, buffer + value.length() );
	}

	static bool deserialize(const Body& data, std::string& result)
	{
		DH_VERIFY(data.size() <= 8);
		result.assign( reinterpret_cast<const char*>(data.data()), data.size() );
		return true;
	}
};



}
}
