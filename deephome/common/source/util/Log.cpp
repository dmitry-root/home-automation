#include "util/Log.h"
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <cstdint>
#include <mutex>
#include <fstream>
#include <iostream>
#include <iomanip>


namespace dh
{
namespace util
{

namespace
{

std::mutex guard_;
uint32_t initial_targets_ = 0;
std::string syslog_ident_;
std::string file_name_;
Logger::Level log_level_ = Logger::Level_Debug;

class LoggerImpl : NonCopyable
{
public:
	LoggerImpl();
	~LoggerImpl();

	void write_line(Logger::Level level, const std::string& line);

private:
	uint32_t targets_;
	std::ofstream file_;
};


LoggerImpl::LoggerImpl()
{
	targets_ = initial_targets_;
	if (targets_ == 0)
		targets_ = Logger::Target_Stderr;

	if (targets_ & Logger::Target_Syslog)
		::openlog(syslog_ident_.c_str(), LOG_PID, LOG_USER);

	if (targets_ & Logger::Target_File)
	{
		file_.open(file_name_.c_str());
		if (file_.bad())
			targets_ &= ~Logger::Target_File;
	}
}

LoggerImpl::~LoggerImpl()
{
	if (targets_ & Logger::Target_Syslog)
		::closelog();

	if (targets_ & Logger::Target_File)
		file_.close();
}

const char* level_to_string(Logger::Level level)
{
	switch (level)
	{
		case Logger::Level_Debug: return "DEBUG";
		case Logger::Level_Info:  return "INFO";
		case Logger::Level_Warning: return "WARNING";
		case Logger::Level_Error: return "ERROR";
		case Logger::Level_FatalError: return "FATAL";
		default: return "unknown";
	}
}

int level_to_syslog(Logger::Level level)
{
	switch (level)
	{
		case Logger::Level_Debug: return LOG_DEBUG;
		case Logger::Level_Info: return LOG_INFO;
		case Logger::Level_Warning: return LOG_WARNING;
		case Logger::Level_Error: return LOG_ERR;
		case Logger::Level_FatalError: return LOG_CRIT;
		default: return LOG_NOTICE;
	}
}

void LoggerImpl::write_line(Logger::Level level, const std::string& line)
{
	if (targets_ & (Logger::Target_Stderr | Logger::Target_File))
	{
		struct ::timeval tv;
		::gettimeofday(&tv, 0);

		struct ::tm localtm;
		::localtime_r(&tv.tv_sec, &localtm);
		const unsigned int msec = tv.tv_usec / 1000;

		char timebuf[256];
		::strftime(timebuf, sizeof(timebuf), "%F %T", &localtm);

		const char* const slevel = level_to_string(level);

		std::ostringstream ss;
		ss.fill('0');
		ss << "[" << timebuf << "." << std::setw(3) << msec << std::setw(0) << "] (" << slevel << "): " << line << std::endl;

		if (targets_ & Logger::Target_Stderr)
			std::cerr << ss.str();

		if (targets_ & Logger::Target_File)
		{
			file_ << ss.str();
			file_.flush();
		}
	}

	if (targets_ & Logger::Target_Syslog)
	{
		::syslog(level_to_syslog(level), "%s", line.c_str());
	}
}

} // anonymous namespace


void Logger::add_target(Target target, const std::string& key)
{
	std::unique_lock<std::mutex> lock(guard_);

	initial_targets_ |= target;
	if (target == Target_Syslog)
		syslog_ident_ = key;
	if (target == Target_File)
		file_name_ = key;
}

void Logger::write_line(Level level, const std::string& line)
{
	std::unique_lock<std::mutex> lock(guard_);

	if (level < log_level_)
		return;

	static LoggerImpl logger;

	logger.write_line(level, line);
}

void Logger::set_level(Level level)
{
	std::unique_lock<std::mutex> lock(guard_);
	log_level_ = level;
}


LogWriter::LogWriter(Logger::Level level, const std::string& file, int line, const std::string& func) :
    level_(level)
{
	init_buffer(file, line, std::string(), func, 0);
}

LogWriter::LogWriter(Logger::Level level, const std::string& file, int line, const std::string& clsname, const std::string& func) :
    level_(level)
{
	init_buffer(file, line, clsname, func, 0);
}

LogWriter::LogWriter(Logger::Level level, const std::string& file, int line, const std::string& clsname, const std::string& func, void* self) :
    level_(level)
{
	init_buffer(file, line, clsname, func, self);
}


std::ostream& LogWriter::output_buffer()
{
	return buffer_;
}


LogWriter::~LogWriter()
{
	Logger::write_line(level_, buffer_.str());
}


void LogWriter::init_buffer(const std::string& file, int line, const std::string& clsname, const std::string& func, void* self)
{
	const size_t fsep = file.find_last_of('/');
	const char* const filename = file.c_str() + (fsep == std::string::npos ? 0 : fsep + 1);

	buffer_ << filename << ":" << line << ", ";
	if (!clsname.empty())
		buffer_ << clsname << "::";
	buffer_ << func;
	if (self)
		buffer_ << "^" << self;
	buffer_ << " ::: ";
}

}
}
