#pragma once

#include <string>

namespace dh
{
namespace util
{

class Runtime
{
public:
	static std::string install_prefix();
	static std::string config_dir();
	static std::string config_file_path(const std::string& file_name);

private:
	Runtime() = delete;
	Runtime(const Runtime&) = delete;
	Runtime& operator= (const Runtime&) = delete;
};

}
}
