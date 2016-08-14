#pragma once

#include <memory>

#include "util/EventLoop.h"
#include "util/Log.h"


namespace dh
{
namespace router
{

class Config;
class Router;


class Application : util::NonCopyable
{
public:
	Application(int argc, char** argv);
	~Application();

	int run();

private:
	void parse_arguments(int argc, char** argv);
	bool parse_loglevel(const std::string& level);
	void print_usage();
	void init_log();

	bool start();
	void stop();

	void on_signal(int signum);

	void read_config();

private:
	const std::string app_name_;
	bool valid_arguments_ = false;
	bool print_help_ = false;
	bool daemonize_ = false;
	std::string log_file_;
	util::Logger::Level log_level_ = util::Logger::Level_Info;
	std::string config_file_;

	util::EventLoop event_loop_;
	util::SignalListener sig_hup_;
	util::SignalListener sig_term_;
	util::SignalListener sig_int_;
	std::unique_ptr<Config> config_;
	std::unique_ptr<Router> router_;
};

}
}
