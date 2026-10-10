#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

/// helper macro for unpacking @code glm::vec3@endcode for log printing
#define LOG_UNPACK_GLMVEC3(v) v.x, v.y, v.z

namespace Log {

    /**
     * @todo could add more logging channels here after (some ideas: UI)
     */
    enum class Channel: uint16_t {
        ENGINE      = 0,
        GAME        = 1,
        MAPPING     = 2,
        PHYSICS     = 3,
        RENDERER    = 4,
        PLAYER      = 5,
    };

    namespace impl {
        std::shared_ptr<spdlog::logger> getLoggerChannel(Channel channel);
    }

    /// NOT SAFE TO CALL TWICE
    void init(spdlog::level::level_enum outputLevel);

    template <typename... Args>
    void trace(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->trace(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->debug(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void info(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->info(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->warn(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->error(std::format(fmt, std::forward<Args>(args)...));
    }
    template <typename... Args>
    void critical(const Channel channel, std::format_string<Args...> fmt, Args&&... args) {
        impl::getLoggerChannel(channel)->critical(std::format(fmt, std::forward<Args>(args)...));
    }

}