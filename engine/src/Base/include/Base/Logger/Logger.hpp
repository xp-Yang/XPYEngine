#ifndef Logger_hpp
#define Logger_hpp

#include <memory>
#include <string>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Logger
{

    class Logger
    {
    public:
        static Logger &get()
        {
            static Logger instance;
            return instance;
        }

        void setLogDirectory(const std::string &dir)
        {
            m_directory = dir;
        }

        template <typename... Args>
        void trace(const char *fmt, const Args &...args) { m_logger->trace(fmt, args...); }

        template <typename... Args>
        void debug(const char *fmt, const Args &...args) { m_logger->debug(fmt, args...); }

        template <typename... Args>
        void info(const char *fmt, const Args &...args) { m_logger->info(fmt, args...); }

        template <typename... Args>
        void warn(const char *fmt, const Args &...args) { m_logger->warn(fmt, args...); }

        template <typename... Args>
        void error(const char *fmt, const Args &...args) { m_logger->error(fmt, args...); }

    protected:
        void initFilepath()
        {
            struct tm *cur_time;
            time_t local_time;
            time(&local_time);
            cur_time = localtime(&local_time);

            /* 通过时间命名日志文件 */
            char filepath[256];
            strftime(filepath, 100, "%Y-%m-%d_%H-%M-%S.log", cur_time);

            /* log file name, does not contain file path */
            std::string log_filepath = std::string(filepath);

            /* 日志文件全路径 */
            m_full_filepath = m_directory + log_filepath;

            /* 记录日志创建时间，在通过时间长度控制日志文件大小时，该成员变量会被使用 */
            // logger_create_time = local_time;
        }

        void initLogger()
        {
            initFilepath();
            std::vector<spdlog::sink_ptr> sinks;
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%Y-%m-%d %T]%^[%l]%$ %v");
            sinks.push_back(console_sink);
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(m_full_filepath.c_str(), true);
            file_sink->set_pattern("[%Y-%m-%d %T]%^%^[%l]%$%$ %v");
            sinks.push_back(file_sink);
            m_logger = std::make_shared<spdlog::logger>("multi_sink", begin(sinks), end(sinks));
            m_logger->set_level(spdlog::level::trace);
            spdlog::register_logger(m_logger);
        }

    private:
        Logger() { initLogger(); }
        std::shared_ptr<spdlog::logger> m_logger;
        std::string m_directory;
        std::string m_full_filepath;
    };

    template <typename... Args>
    void trace(const char *fmt, const Args &...args)
    {
        Logger::get().trace(fmt, args...);
    }

    template <typename... Args>
    void debug(const char *fmt, const Args &...args)
    {
        Logger::get().debug(fmt, args...);
    }

    template <typename... Args>
    void info(const char *fmt, const Args &...args)
    {
        Logger::get().info(fmt, args...);
    }

    template <typename... Args>
    void warn(const char *fmt, const Args &...args)
    {
        Logger::get().warn(fmt, args...);
    }

    template <typename... Args>
    void error(const char *fmt, const Args &...args)
    {
        Logger::get().error(fmt, args...);
    }

} // namespace Logger

#endif
