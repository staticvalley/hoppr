#include "Log.hpp"

namespace Log {

    namespace impl {

        const std::unordered_map<Channel, std::string> channels = {
            {Channel::ENGINE,   "engine"},
            {Channel::GAME,     "game"},
            {Channel::MAPPING,  "mapping"},
            {Channel::PHYSICS,  "physics"},
            {Channel::RENDERER, "renderer"},
            {Channel::PLAYER,   "player"},
            {Channel::MDL,      "mdl_loader"}
        };

        std::shared_ptr<spdlog::logger> getLoggerChannel(Channel channel) {
            auto logger = spdlog::get(channels.at(channel));
            if (logger == nullptr) {
                auto engine = spdlog::default_logger();
                engine->warn("Logger channel could not be found, outputting to default channel");
                return engine;
            }
            return logger;
        }

        static std::shared_ptr<spdlog::logger> createChannel(const std::string& loggerName, std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file) {
            auto logger = spdlog::stdout_color_mt(loggerName);
            logger->sinks().push_back(file);
            return logger;
        }
    }

    void init(spdlog::level::level_enum outputLevel) {
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file    = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/run.log", true);

        auto engineLog = std::make_shared<spdlog::logger>(impl::channels.at(Channel::ENGINE), spdlog::sinks_init_list{console, file});
        spdlog::set_default_logger(engineLog);
        spdlog::set_level(outputLevel);
        spdlog::flush_on(spdlog::level::warn);

        for (const auto& [channelEnum, name]: impl::channels) {
            // skips first "engine" channel as its already created
            if (channelEnum == Channel::ENGINE) continue;
            impl::createChannel(name, file);
        }

    }
}

