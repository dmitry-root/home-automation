#pragma once

#include <map>
#include <string>
#include <sstream>

#include "NonCopyable.h"


namespace dh
{
namespace util
{

class Config : NonCopyable
{
public:
	Config();
	Config(const std::string& name);
	~Config() = default;

	void read(const std::string& name);
	void read_file(const std::string& file_name);

	typedef std::map<std::string, std::string> ValueMap;

	const ValueMap& values() const;

	bool try_get(const std::string& key, std::string& result) const;

	template <typename T>
	inline T get(const std::string& key, const T& default_value = T()) const;

private:
	ValueMap values_;
};



template <typename T>
inline T Config::get(const std::string& key, const T& default_value) const
{
	std::string value;
	if (!try_get(key, value))
		return default_value;

	T result = default_value;
	std::istringstream(value) >> result;
	return result;
}

template <>
inline std::string Config::get<std::string>(const std::string& key, const std::string& default_value) const
{
	std::string result;
	if (try_get(key, result))
		return result;
	else
		return default_value;
}

}
}
