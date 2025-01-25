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

        [[nodiscard]] std::shared_ptr<spdlog::logger> get_system_logger();
        [[nodiscard]] std::shared_ptr<spdlog::logger> get_network_logger();
        [[nodiscard]] std::shared_ptr<spdlog::logger> get_request_logger();

        ~logger();
    private:
        std::shared_ptr<spdlog::logger> m_system_logger;
        std::shared_ptr<spdlog::logger> m_request_logger;
        std::shared_ptr<spdlog::logger> m_network_logger;
    };

    std::shared_ptr<spdlog::logger> logger::get_system_logger()
    {
        return m_system_logger;
    }

    std::shared_ptr<spdlog::logger> logger::get_network_logger()
    {
        return m_network_logger;
    }

    std::shared_ptr<spdlog::logger> logger::get_request_logger()
    {
        return m_request_logger;
    }
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
    m_request_logger = std::make_shared<spdlog::logger>("Request", sinks.begin(), sinks.end());
    m_network_logger = std::make_shared<spdlog::logger>("Network", sinks.begin(), sinks.end());

    m_system_logger->set_level(spdlog::level::info);
    m_request_logger->set_level(spdlog::level::info);
    m_network_logger->set_level(spdlog::level::info);

    spdlog::register_logger(m_system_logger);
    spdlog::register_logger(m_request_logger);
    spdlog::register_logger(m_network_logger);

    //spdlog::set_default_logger(m_system_logger);

    spdlog::cfg::load_argv_levels(argc, argv);

    m_system_logger->info("Logging engine initialized");
}

LambdaSnail::logging::logger::~logger()
{
    m_system_logger->info("Shutting down logging system");

    m_system_logger->flush();
    m_request_logger->flush();
    m_network_logger->flush();

    spdlog::shutdown();
};
