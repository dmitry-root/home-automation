#include <fstream>
#include <stdexcept>

#include "util/Assert.h"
#include "util/Config.h"
#include "util/Runtime.h"
#include "util/Log.h"


namespace dh
{
namespace util
{

namespace
{

const std::string space = " \t\r";

void strip_comment(std::string& line)
{
	const size_t pos = line.find('#');
	if (pos != std::string::npos)
		line.replace(pos, line.length() - pos, std::string());
}

std::string trim(const std::string& line)
{
	const size_t first = line.find_first_not_of(space);
	if (first == std::string::npos)
		return std::string();

	const size_t last = line.find_last_not_of(space);
	DH_ASSERT( last != std::string::npos );

	return line.substr(first, last - first);
}

void strip_space(std::string& line)
{
	line = trim(line);
}

}

#define DH_LOG_CLASS "Config"

Config::Config()
{
}

Config::Config(const std::string& name)
{
	read(name);
}

void Config::read(const std::string& name)
{
	read_file( Runtime::config_file_path(name + ".conf"));
}

void Config::read_file(const std::string& file_name)
{
	DH_LOG(Info) << "Configuration file: " << file_name;

	std::ifstream in(file_name);
	if (!in)
	{
		DH_LOG(Warning) << "Configuration file not found: " << file_name;
		throw std::runtime_error("Configuration file not found: " + file_name);
	}

	int lineno = 0;
	std::string line;
	std::string section_name;
	while (std::getline(in, line))
	{
		++lineno;
		strip_comment(line);
		strip_space(line);

		if (line.empty())
			continue; // skip empty line

		if (line[0] == '[')
		{
			const size_t end = line.find_last_of(']');
			if (end != line.length() - 1)
			{
				DH_LOG(FatalError) << "Syntax error at line " << lineno << ": wrong section syntax";
				throw std::runtime_error("Syntax error in configuration file (wrong section syntax)");
			}

			section_name = trim( line.substr(1, line.length() - 2) );
			continue;
		}

		const size_t sep = line.find('=');
		if (sep == std::string::npos)
		{
			DH_LOG(FatalError) << "Syntax error at line " << lineno << ": value not specified";
			throw std::runtime_error("Syntax error in configuration file (value not specified)");
		}

		const std::string local_key = trim( line.substr(0, sep) );
		const std::string key = section_name.empty() ? local_key : (section_name + "." + local_key);
		const std::string value = trim( line.substr(sep + 1) );

		values_.insert( std::make_pair(key, value) );
	}
}

const Config::ValueMap& Config::values() const
{
	return values_;
}

bool Config::try_get(const std::string& key, std::string& result) const
{
	const ValueMap::const_iterator it = values_.find(key);
	if (it == values_.cend())
		return false;

	result = it->second;
	return true;
}

}
}
