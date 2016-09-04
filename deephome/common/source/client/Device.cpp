#include <algorithm>
#include <memory>

#include "util/Assert.h"
#include "client/Device.h"
#include "client/Dispatcher.h"

#include "ha/can_proto.h"


namespace dh
{
namespace client
{


namespace
{

class SerialBuilder;
typedef std::shared_ptr<SerialBuilder> SerialBuilderPtr;

class SerialBuilder : util::NonCopyable
{
public:
	enum Part
	{
		Part_Hi,
		Part_Lo
	};

	typedef typename Result<Device::SerialNumber>::ValueType ValueType;

	SerialBuilder(Result<Device::SerialNumber>& result) :
	    result_(result)
	{
		serial_.fill(0);
	}

private:
	friend void handle_serial(SerialBuilderPtr builder, Part part, const Result<Body>::ValueType& value);

	void handle_result(Part part, const Result<Body>::ValueType& value)
	{
		if (complete_)
			return; // result already notified

		if (!value || value->size() != 8)
		{
			// timeout occured first time, report it
			result_.set( ValueType() );
			complete_ = true;
			return;
		}

		if (part == Part_Hi && !hi_)
		{
			std::copy(value->begin(), value->end(), serial_.begin());
			hi_ = true;
		}
		else if (part == Part_Lo && !lo_)
		{
			std::copy(value->begin(), value->end(), serial_.begin() + 8);
			lo_ = true;
		}

		if (hi_ && lo_)
		{
			result_.set(serial_);
			complete_ = true;
		}
	}

private:
	Result<Device::SerialNumber>& result_;
	Device::SerialNumber serial_;
	bool hi_ = false;
	bool lo_ = false;
	bool complete_ = false;
};

void handle_serial(SerialBuilderPtr builder, SerialBuilder::Part part, const Result<Body>::ValueType& value)
{
	DH_ASSERT( builder );
	builder->handle_result(part, value);
}

}


Device::Device(Dispatcher& dispatcher, const std::string& name) :
    dispatcher_(dispatcher),
    name_(name)
{
}

Device::~Device()
{
}

Dispatcher& Device::dispatcher()
{
	return dispatcher_;
}

const Dispatcher& Device::dispatcher() const
{
	return dispatcher_;
}

std::string Device::name() const
{
	return name_;
}

uint32_t Device::timeout() const
{
	return timeout_;
}

void Device::set_timeout(uint32_t timeout)
{
	timeout_ = timeout;
}

void Device::query_type(Result<uint32_t>& result)
{
	query(HA_CAN_Common_DeviceType, result);
}

void Device::query_firmware_version(Result<uint16_t>& result)
{
	query(HA_CAN_Common_FirmwareVersion, result);
}

void Device::query_serial(Result<SerialNumber>& result)
{
	SerialBuilderPtr serial_builder = std::make_shared<SerialBuilder>(result);

	using namespace std::placeholders;
	CallbackResult<Body> hi_result( std::bind(handle_serial, serial_builder, SerialBuilder::Part_Hi, _1) );
	CallbackResult<Body> lo_result( std::bind(handle_serial, serial_builder, SerialBuilder::Part_Lo, _1) );

	query(HA_CAN_Common_SerialHi, hi_result);
	query(HA_CAN_Common_SerialLo, lo_result);
}

void Device::query_label(Result<std::string>& result)
{
	query(HA_CAN_Common_DeviceLabel, result);
}

void Device::assign_label(const std::string& label)
{
	send(HA_CAN_Common_DeviceLabel, label);
}

}
}
