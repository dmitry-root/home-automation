#pragma once

#include <array>
#include <stdexcept>

#include "util/NonCopyable.h"
#include "client/CommonTypes.h"
#include "client/Result.h"
#include "client/Dispatcher.h"


namespace dh
{
namespace client
{

class Device : util::NonCopyable
{
public:
	Device(Dispatcher& dispatcher, const std::string& name);
	~Device();

	Dispatcher& dispatcher();
	const Dispatcher& dispatcher() const;

	std::string name() const;

	uint32_t timeout() const;
	void set_timeout(uint32_t timeout);

	typedef std::array<uint8_t, 16> SerialNumber;

	void query_type(Result<uint32_t>& result);
	void query_firmware_version(Result<uint16_t>& result);
	void query_serial(Result<SerialNumber>& result);
	void query_label(Result<std::string>& result);

	void assign_label(const std::string& label);


	template <typename T>
	void query(uint16_t address, Result<T>& result)
	{
		const DeviceCommand command(name(), DeviceCommand::Type_Query, address);
		dispatcher().send_query(command, result, timeout());
	}

	template <typename T>
	void send(uint16_t address, const T& value)
	{
		Body body;
		Serializer<T>::serialize(value, body);

		const DeviceCommand command(name(), DeviceCommand::Type_Send, address, body);
		dispatcher().send(command);
	}

private:
	Dispatcher& dispatcher_;
	const std::string name_;
	uint32_t timeout_ = infinite_timeout;
};


namespace sync
{

class Timeout : public std::runtime_error
{
public:
	Timeout() : std::runtime_error("Timeout") {}
};


template <typename T, typename Dev, void (Dev::*QueryProc)(Result<T>&)>
inline T get(Dev& dev)
{
	typedef BlockingResult<T> MyResult;
	typedef typename MyResult::ValueType MyValue;

	MyResult result;
	(dev.*QueryProc)(result);
	MyValue value = result.wait();

	if (!value)
		throw Timeout();
	return *value;
}


inline uint32_t get_device_type(Device& device)
{
	return get<uint32_t, Device, &Device::query_type>(device);
}

inline uint16_t get_device_firmware_version(Device& device)
{
	return get<uint16_t, Device, &Device::query_firmware_version>(device);
}

inline Device::SerialNumber get_device_serial(Device& device)
{
	return get<Device::SerialNumber, Device, &Device::query_serial>(device);
}

inline std::string get_device_label(Device& device)
{
	return get<std::string, Device, &Device::query_label>(device);
}

}

}
}
