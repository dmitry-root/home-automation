#include <cstdlib>
#include <unistd.h>

#include "util/Runtime.h"


namespace dh
{
namespace util
{

namespace
{
const std::string prefix_env = "DEEPHOME_PATH";
}

std::string Runtime::install_prefix()
{
	// 1. check environment variable
	if (const char* const prefix = std::getenv(prefix_env.c_str()))
	{
		const std::string result(prefix);
		if (!result.empty())
		{
			if (result[result.length()-1] == '/')
				return result.substr(0, result.length() - 1);
			else
				return result;
		}
	}

	// 2. guess path by current process dir
	{
		char exepath[512];
		const ssize_t linklen = ::readlink("/proc/self/exe", exepath, sizeof(exepath));
		if (linklen > 0)
		{
			std::string path(exepath, linklen);

			// strip executable name
			const size_t pos = path.find_last_of('/');
			if (pos != std::string::npos)
				path.replace(pos, path.length() - pos, std::string());

			// strip /bin dir name
			static const std::string bindir = "/bin";
			if (path.length() > bindir.length() && path.substr(path.length() - bindir.length(), bindir.length()) == bindir)
				path.replace(path.length() - bindir.length(), bindir.length(), std::string());

			return path;
		}
	}

	// 3. return the random default
	return "/usr/local";
}

std::string Runtime::config_dir()
{
	return install_prefix() + "/etc";
}

std::string Runtime::config_file_path(const std::string& file_name)
{
	return config_dir() + "/" + file_name;
}

}
}
