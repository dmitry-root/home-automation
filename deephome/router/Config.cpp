#include <stdexcept>
#include <set>
#include <sstream>

#include "util/Assert.h"

#include "Config.h"


namespace dh
{
namespace router
{

namespace
{

void extract_keys(const util::Config& config, const std::string& prefix, std::set<std::string>& keys)
{
	const std::string prefix_dot = prefix + ".";
	const size_t prefix_len = prefix_dot.length();

	for (auto item : config.values())
	{
		const std::string key = item.first;
		if (key.substr(0, prefix_len) != prefix_dot)
			continue;

		const size_t end = key.find('.', prefix_len);
		if (end == std::string::npos)
			continue;

		const std::string key_id = key.substr(prefix_len, end - prefix_len);
		keys.insert(key_id);
	}
}

}

#define DH_LOG_CLASS "router::Config"

Config::Config(const util::Config& config)
{
	parse_config(config);
}

void Config::parse_config(const util::Config& config)
{
	parse_networks(config);
	parse_devices(config);
	parse_bindings(config);
}

void Config::parse_networks(const util::Config& config)
{
	std::set<std::string> keys;
	extract_keys(config, "network", keys);
	if (keys.empty())
	{
		DH_LOG(Error) << "no networks defined in configuration";
		throw std::runtime_error("no networks defined in configuration");
	}

	for (const std::string& key : keys)
	{
		const std::string address = config.get<std::string>("network." + key + ".address");
		const std::string service = config.get<std::string>("network." + key + ".port", "2121");

		if (address.empty())
		{
			DH_LOG(Warning) << "no address specified for network: " << key << ", skipped";
			continue;
		}

		NetworkPtr network = std::make_shared<Network>(key, address, service);
		networks_.insert( std::make_pair(key, network) );
	}

	if (networks_.empty())
	{
		DH_LOG(Error) << "no valid networks defined in configuration";
		throw std::runtime_error("no valid networks defined in configuration");
	}
}

void Config::parse_devices(const util::Config& config)
{
	std::set<std::string> keys;
	extract_keys(config, "device", keys);
	if (keys.empty())
	{
		DH_LOG(Error) << "no devices defined in configuration";
		throw std::runtime_error("no devices found in configuration");
	}

	for (const std::string& key : keys)
	{
		const std::string network_name = config.get<std::string>("device." + key + ".network");
		const std::string id_str = config.get<std::string>("device." + key + ".id");

		uint32_t id = 0;
		if (!(std::istringstream(id_str) >> std::hex >> id) || id > 127)
		{
			DH_LOG(Error) << "device id is invalid or out of range, device: " << key;
			throw std::runtime_error("invalid device id");
		}

		const Network* network = find_network(network_name);
		if (!network)
		{
			DH_LOG(Error) << "network name is invalid, device: " << key << ", network: " << network_name;
			throw std::runtime_error("invalid network name");
		}

		DevicePtr device = std::make_shared<Device>(key, network_name, *network, id);
		devices_.insert( std::make_pair(key, device) );
	}
}

void Config::parse_bindings(const util::Config& config)
{
	std::set<std::string> keys;
	extract_keys(config, "bind", keys);
	if (keys.empty())
	{
		DH_LOG(Error) << "no bindings defined in configuration";
		throw std::runtime_error("no bindings found in configuration");
	}

	for (const std::string& key : keys)
	{
		const std::string address = config.get<std::string>("bind." + key + ".address");
		const std::string service = config.get<std::string>("bind." + key + ".port");
		const std::string path = config.get<std::string>("bind." + key + ".path");

		BindingPtr binding;

		if (!service.empty() && path.empty())
			binding = std::make_shared<Binding>(Binding::Type_TCP, address, service);
		else if (address.empty() && service.empty() && !path.empty())
			binding = std::make_shared<Binding>(Binding::Type_UNIX, path);
		else
		{
			DH_LOG(Warning) << "invalid binding: either (address, port) or path must be specified, skipped. Key: " << key;
			continue;
		}

		binding->backlog = config.get<int>("bind." + key + ".backlog", 5);
		bindings_.push_back( binding );
	}
}


const Config::Network* Config::find_network(const std::string& name) const
{
	auto it = networks_.find(name);
	return it == networks_.cend() ? 0 : it->second.get();
}

const Config::Device* Config::find_device(const std::string& name) const
{
	auto it = devices_.find(name);
	return it == devices_.cend() ? 0 : it->second.get();
}

const Config::Device* Config::find_device(const std::string& network_name, uint8_t id) const
{
	for (auto device : devices_)
	{
		if (device.second->network_name == network_name && device.second->id == id)
			return device.second.get();
	}
	return 0;
}

void Config::get_networks(NetworkList& networks) const
{
	NetworkList result;
	result.reserve(networks_.size());

	for (auto network : networks_)
		result.push_back( network.second.get() );

	networks.swap(result);
}

void Config::get_devices(DeviceList& devices) const
{
	DeviceList result;
	result.reserve(devices_.size());

	for (auto device : devices_)
		result.push_back( device.second.get() );

	devices.swap(result);
}

void Config::get_bindings(BindingList& bindings) const
{
	BindingList result;
	result.reserve(bindings_.size());

	for (auto binding : bindings_)
		result.push_back( binding.get() );

	bindings.swap(result);
}


}
}
