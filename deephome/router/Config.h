#pragma once

#include <memory>
#include <vector>

#include "util/NonCopyable.h"
#include "util/Config.h"


namespace dh
{
namespace router
{

class Config : util::NonCopyable
{
public:
	struct Network
	{
		Network(const std::string& name, const std::string& address, const std::string& service) :
		    name(name), address(address), service(service) {}

		std::string name;
		std::string address;
		std::string service;
	};

	struct Device
	{
		Device(const std::string& name, const std::string& network_name, const Network& network, uint8_t id) :
		    name(name), network_name(network_name), network(network), id(id) {}

		std::string name;
		std::string network_name;
		const Network& network;
		uint8_t id;
	};

	struct Binding
	{
		enum Type
		{
			Type_TCP,
			Type_UNIX
		};

		Binding(Type type, const std::string& address, const std::string& service = std::string()) :
		    type(type), address(address), service(service) {}

		Type type;
		std::string address;
		std::string service;
		int backlog = 5;
	};

	typedef std::vector<const Network*> NetworkList;
	typedef std::vector<const Device*> DeviceList;
	typedef std::vector<const Binding*> BindingList;

	Config(const util::Config& config);

	const Network* find_network(const std::string& name) const;
	const Device* find_device(const std::string& name) const;
	const Device* find_device(const std::string& network_name, uint8_t id) const;

	void get_networks(NetworkList& networks) const;
	void get_devices(DeviceList& devices) const;
	void get_bindings(BindingList& bindings) const;

private:
	void parse_config(const util::Config& config);
	void parse_networks(const util::Config& config);
	void parse_devices(const util::Config& config);
	void parse_bindings(const util::Config& config);

private:
	typedef std::shared_ptr<Network> NetworkPtr;
	typedef std::shared_ptr<Device> DevicePtr;
	typedef std::shared_ptr<Binding> BindingPtr;

	typedef std::map<std::string, NetworkPtr> NetworkMap;
	typedef std::map<std::string, DevicePtr> DeviceMap;
	typedef std::vector<BindingPtr> Bindings;

private:
	NetworkMap networks_;
	DeviceMap devices_;
	Bindings bindings_;
};

}
}
