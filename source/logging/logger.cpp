module;

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/cfg/argv.h>

export module logging: logging.logger;

namespace LambdaSnail::logging
{
    export class logger final
    {
    public:
        logger() = default;

        void init_logger(int argc, char const** argv);

        ~logger();
    private:
        std::shared_ptr<spdlog::logger> m_system_logger;
    };
}

void LambdaSnail::logging::logger::init_logger(int argc, char const **argv)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    //console_sink->set_pattern("%^[System] [%l] [%Y-%m-%d %H:%M:S.%e] %v%$");

    auto constexpr max_size = 1048576 * 5;
    auto constexpr max_files = 3;
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/system_log.txt", max_size, max_files);
    file_sink->set_level(spdlog::level::info);

    std::vector<spdlog::sink_ptr> sinks = { console_sink, file_sink };

    m_system_logger = std::make_shared<spdlog::logger>("System", sinks.begin(), sinks.end());
    m_system_logger->set_level(spdlog::level::info);

    spdlog::cfg::load_argv_levels(argc, argv);
    spdlog::register_logger(m_system_logger);

    m_system_logger->info("Logging engine initialized");
}

LambdaSnail::logging::logger::~logger()
{
    m_system_logger->info("Shutting down logging system");
    spdlog::shutdown();
};
