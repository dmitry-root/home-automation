#pragma once

#include <string>
#include <sstream>

#include "NonCopyable.h"

namespace dh
{
namespace util
{

class Logger : NonCopyable
{
public:
	enum Level
	{
		Level_Debug,
		Level_Info,
		Level_Warning,
		Level_Error,
		Level_FatalError
	};

	enum Target
	{
		Target_Syslog = 0x01,
		Target_Stderr = 0x02,
		Target_File = 0x04,
		Target_Null = 0x08
	};

	static void add_target(Target target, const std::string& key);
	static void write_line(Level level, const std::string& line);

private:
	Logger();
	~Logger();
};

class LogWriter : NonCopyable
{
public:
	LogWriter(Logger::Level level, const std::string& file, int line, const std::string& func);
	LogWriter(Logger::Level level, const std::string& file, int line, const std::string& clsname, const std::string& func);
	LogWriter(Logger::Level level, const std::string& file, int line, const std::string& clsname, const std::string& func, void* self);
	~LogWriter();

	std::ostream& output_buffer();

	template <typename T>
	LogWriter& operator << (const T& value)
	{
		output_buffer() << value;
		return *this;
	}

private:
	void init_buffer(const std::string& file, int line, const std::string& clsname, const std::string& func, void* self);

private:
	const Logger::Level level_;
	std::ostringstream buffer_;
};

}
}


#define DH_LOG(level) \
	dh::util::LogWriter( dh::util::Logger::Level_##level, __FILE__, __LINE__, DH_LOG_CLASS, __FUNCTION__, this)

#define DH_SLOG(level) \
	dh::util::LogWriter( dh::util::Logger::Level_#level, __FILE__, __LINE__, DH_LOG_CLASS, __FUNCTION__)

#define DH_GLOG(level) \
	dh::util::LogWriter( dh::util::Logger::Level_##level, __FILE__, __LINE__, __FUNCTION__)

