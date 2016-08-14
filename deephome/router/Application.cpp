#include <iostream>

#include "util/Assert.h"
#include "util/Config.h"

#include "Config.h"
#include "Router.h"
#include "Application.h"


namespace dh
{
namespace router
{

#define DH_LOG_CLASS "Application"

Application::Application(int argc, char** argv) :
    app_name_(argv[0]),
    sig_hup_(event_loop_, SIGHUP, std::bind(&Application::on_signal, this, SIGHUP)),
    sig_term_(event_loop_, SIGTERM, std::bind(&Application::on_signal, this, SIGTERM)),
    sig_int_(event_loop_, SIGINT, std::bind(&Application::on_signal, this, SIGINT))
{
	parse_arguments(argc, argv);
}

Application::~Application()
{
}

int Application::run()
{
	if (!valid_arguments_ || print_help_)
	{
		print_usage();
		return valid_arguments_ ? 0 : 1;
	}

	init_log();

	if (!start())
		return 2;

	event_loop_.run();

	stop();

	return 0;
}


bool Application::start()
{
	try
	{
		read_config();
		router_.reset( new Router(event_loop_, *config_) );
		router_->start();
	}
	catch (const std::exception& e)
	{
		DH_LOG(FatalError) << "router startup failed: " << e.what();
		return false;
	}

	sig_hup_.start();
	sig_term_.start();
	sig_int_.start();

	return true;
}

void Application::stop()
{
	sig_hup_.stop();
	sig_term_.stop();
	sig_int_.stop();

	router_->stop();
	router_.reset();
	config_.reset();
}

void Application::on_signal(int signum)
{
	if (signum == SIGHUP)
	{
		DH_LOG(Info) << "got SIGHUP, restarting service";

		stop();

		if (start())
			return;
	}

	DH_LOG(Warning) << "terminate on signal " << signum;
	event_loop_.unloop();
}

void Application::read_config()
{
	DH_VERIFY( router_.get() == 0 );

	util::Config raw_config;

	if (config_file_.empty())
	{
		DH_LOG(Info) << "reading default configuration file";
		raw_config.read("dhrouter");
	}
	else
	{
		DH_LOG(Info) << "reading custom configuration file: " << config_file_;
		raw_config.read_file(config_file_);
	}

	config_.reset( new Config(raw_config) );
}


void Application::parse_arguments(int argc, char** argv)
{
	enum State
	{
		State_Keys,
		State_Logfile,
		State_Loglevel,
		State_ConfigFile
	} state = State_Keys;

	for (int i = 1; i < argc; ++i)
	{
		const std::string arg = argv[i];
		if (arg.empty())
			return;

		if (arg[0] == '-' && state == State_Keys)
		{
			if (arg == "-d")
				daemonize_ = true;
			else if (arg == "-h" || arg == "-?" || arg == "--help")
				print_help_ = true;
			else if (arg == "--")
				state = State_ConfigFile;
			else if (arg == "-L" || arg == "--log")
				state = State_Logfile;
			else if (arg == "-l" || arg == "--level")
				state = State_Loglevel;
			else
				return;
		}
		else if (state == State_Logfile)
		{
			log_file_ = arg;
			state = State_Keys;
		}
		else if (state == State_Loglevel)
		{
			if (!parse_loglevel(arg))
				return;
			state = State_Keys;
		}
		else
		{
			if (!config_file_.empty())
				return;
			config_file_ = arg;
		}
	}

	if (state == State_Keys || (state == State_ConfigFile && !config_file_.empty()))
		valid_arguments_ = true;
}

static bool matches(const std::string& expected, const std::string& value)
{
	if (value.length() > expected.length())
		return false;

	for (size_t i = 0, n = value.length(); i < n; ++i)
		if (std::tolower(value[i]) != std::tolower(expected[i]))
			return false;

	return true;
}

bool Application::parse_loglevel(const std::string& level)
{
	if (level.empty())
		return false;

	if (matches("debug", level) || level == "0")
		log_level_ = util::Logger::Level_Debug;
	else if (matches("info", level) || level == "1")
		log_level_ = util::Logger::Level_Info;
	else if (matches("warning", level) || level == "2")
		log_level_ = util::Logger::Level_Warning;
	else if (matches("error", level) || level == "3")
		log_level_ = util::Logger::Level_Error;
	else if (matches("fatal", level) || level == "4")
		log_level_ = util::Logger::Level_FatalError;
	else
		return false;

	return true;
}

void Application::print_usage()
{
	std::cerr <<
	             "Usage:\n"
	             "    " << app_name_ << " [-d] [-h] [-L log_file] [-l level] [--] [config_file]\n\n"
	             "Arguments:\n"
	             "    -d           run in daemon mode\n"
	             "    -h           print this help\n"
	             "    -L log_file  write log into specified file instead of syslog\n"
	             "    -l level     specify log level: debug, info, warning, error, fatal\n"
	             "    config_file  run with specific configuration file\n";
}

void Application::init_log()
{
	util::Logger::set_level(log_level_);

	if (!log_file_.empty())
	{
		if (log_file_ == "-")
			util::Logger::add_target(util::Logger::Target_Stderr, std::string());
		else
			util::Logger::add_target(util::Logger::Target_File, log_file_);
	}
	else
	{
		util::Logger::add_target(util::Logger::Target_Syslog, "dhrouter");
	}
}

}
}
