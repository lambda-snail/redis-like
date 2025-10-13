module;

#include <cassert>
#include <cmath>
#include <expected>
#include <format>
#include <stack>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

#include <tracy/Tracy.hpp>

/**
 * The Tracy macros for instrumenting a block are not compatible with constexpr, so in
 * order to profile code marked as constexpr, the keyword needs to be disabled when the
 * profiler is in use.
 */
#ifndef TRACY_ENABLE
#define profile_constexpr constexpr
#else
#define profile_constexpr
#endif

export module resp:resp.parser;

namespace LambdaSnail::resp
{
    export enum class data_type : uint8_t {
        SimpleString = '+',
        SimpleError  = '-',
        Integer      = ':',
        Boolean      = '#',
        Double       = ',',
        Null         = '_',
        Array        = '*',
        BulkString   = '$'
    };

    export inline namespace literals
    {
        constexpr std::string resp_end  = "\r\n";
        constexpr std::string resp_null = "_\r\n";

        constexpr std::string operator""_resp_error(char const* str, size_t const len)
        {
            return static_cast<char>(data_type::SimpleError) + std::string(str, len) + resp_end;
        }

        constexpr std::string operator""_resp_simple_string(char const* str, size_t const len)
        {
            return static_cast<char>(data_type::SimpleString) + std::string(str, len) + resp_end;
        }

        constexpr std::string resp_ok = "OK"_resp_simple_string;
    }

    struct resp_traits
    {
        static constexpr size_t MaxStringSizeBytes = 512 * 1024 * 1024;
    };
} // namespace LambdaSnail::resp

namespace LambdaSnail::resp::v2
{
    export enum struct parse_errc
    {
        Success = 0,
        UnknownRespType,
        InvalidToken,
        EmptyString,
        PayloadTooLarge
    };
}

namespace std
{
    template <> struct is_error_code_enum<LambdaSnail::resp::v2::parse_errc> : true_type {};
}

namespace LambdaSnail::resp::v2
{
    export class parse_errc_category : public std::error_category
    {
    public:
        [[nodiscard]] virtual const char *name() const noexcept override final { return "ParseError"; }

        [[nodiscard]] virtual std::string message(int c) const override final
        {
            switch (static_cast<parse_errc>(c))
            {
                case parse_errc::Success:
                    return "Parse successful";
                case parse_errc::UnknownRespType:
                    return "Unknown data type";
                case parse_errc::EmptyString:
                    return "Encountered empty string while parsing";
                case parse_errc::InvalidToken:
                    return "Encountered an invalid token for the given data type";
                case parse_errc::PayloadTooLarge:
                    return std::format("The payload is too large. The maximum payload is {} MiB", resp_traits::MaxStringSizeBytes);
                default:
                    return "unknown";
            }
        }

        [[nodiscard]] virtual std::error_condition default_error_condition(int c) const noexcept override final
        {
            switch (static_cast<parse_errc>(c))
            {
                case parse_errc::InvalidToken:
                    return make_error_condition(std::errc::invalid_argument);
                case parse_errc::UnknownRespType:
                    return make_error_condition(std::errc::operation_not_supported);
                case parse_errc::EmptyString:
                    return make_error_condition(std::errc::invalid_argument);
                case parse_errc::PayloadTooLarge:
                    return make_error_condition(std::errc::message_size);
                default:
                    return {c, *this};
            }
        }
    };
}

static LambdaSnail::resp::v2::parse_errc_category const& parse_errc_category()
{
    static LambdaSnail::resp::v2::parse_errc_category c;
    return c;
}

namespace LambdaSnail::resp::v2
{
    std::error_code make_error_code(LambdaSnail::resp::v2::parse_errc e)
    {
        return {static_cast<int>(e), parse_errc_category()};
    }

    export struct null
    {
        bool operator==(null const&) const { return true; }
    };

    export typedef std::variant<int64_t, std::string, double, bool, null> data;

    class stateful_parser
    {
    public:
        explicit stateful_parser(char const prefix) : m_prefix(prefix) {}

        [[nodiscard]] virtual bool is_done() const { return m_is_fully_parsed; }
        [[nodiscard]] virtual std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                           std::vector<data>& data_) = 0;

        virtual ~stateful_parser() = default;

    protected:
        char const m_prefix;
        bool m_is_fully_parsed { false };
    };

    class int_parser : public stateful_parser
    {
    public:
        explicit int_parser(char const prefix = static_cast<char>(data_type::Integer)) : stateful_parser(prefix) {}

        //[[nodiscard]] data get_value() const override { assert(is_fully_parsed); return data{ state }; }
        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;

        // int_parser(int_parser&& parser) noexcept = delete;
        // int_parser(const int_parser& parser) = delete;
        // int_parser& operator=(int_parser const&) const = delete;
        // int_parser& operator=(int_parser const&&) = delete;
    private:
        int64_t m_state {}; // Intermediate or fully parsed value
        bool m_is_negative { false };
    };

    class double_parser : public stateful_parser
    {
    public:
        explicit double_parser() : stateful_parser(static_cast<char>(data_type::Double)) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;

    private:
        double m_state {};
        double m_fraction {};
        double m_power { 1 };
        bool m_is_fraction { false };
        bool m_is_negative { false };
    };

    class boolean_parser : public stateful_parser
    {
    public:
        explicit boolean_parser() : stateful_parser(static_cast<char>(data_type::Boolean)) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;
    private:
        bool m_state { false };
    };

    class null_parser : public stateful_parser
    {
    public:
        explicit null_parser() : stateful_parser(static_cast<char>(data_type::Null)) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;
    };

    class simple_string_parser final : public stateful_parser
    {
    public:
        explicit simple_string_parser() : stateful_parser(static_cast<char>(data_type::SimpleString)) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;

    private:
        std::string m_state {};
    };

    class bulk_string_parser final : public stateful_parser
    {
    public:
        explicit bulk_string_parser() :
            stateful_parser(static_cast<char>(data_type::BulkString)),
            m_size_parser(static_cast<char>(data_type::BulkString)) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;

    private:

        int_parser m_size_parser;

        size_t m_parsed_line_ending { resp_end.size() };
        size_t m_size { 0 };
        std::string m_state {};

        int_parser m_num_parser = int_parser(static_cast<char>(data_type::BulkString));
    };

    /**
     * Online parser that can be called incrementally to parse a message in chunks.
     *
     * To simplify things for this exercise, it has been assumed that all messages are encased in
     * an array. This assumption is reasonable since RESP commands are sent as arrays (see
     * https://redis.io/docs/latest/develop/reference/protocol-spec/#arrays) so the use case for the
     * parser is actually to parse RESP arrays.
     *
     * However, this is not valid in general for RESP, as arrays can contain nested arrays arbitrarily.
     */
    // TODO: Write tests for parsing without type marker in message - should not segfault
    export class parser
    {
    public:
        [[nodiscard]] profile_constexpr std::expected<size_t, std::error_code> add_buffer(std::string_view buffer, std::vector<data>& data_);

        void reset();
        [[nodiscard]] inline bool is_done() const { return m_is_done; };

        void set_expected_num_elements(size_t num) { m_num_elements = num; };

    private:
        size_t m_num_elements { 1 };
        bool m_is_done { false };

        std::stack<std::shared_ptr<stateful_parser>> m_parsers {};
        std::error_code add_parser(std::string_view::const_iterator start);
    };

    /**
     * The array parser will always run first in a well-formed message. To make this work, the array parser
     * communicates to the parser how many elements are expected to appear in the message. This is slightly
     * convoluted but works for the purposes of this limited scenario where we know that there first thing
     * to parse is always an array, and no nested arrays exist.
     */
    // TODO: Use int parser as member instead
    class array_parser final : public int_parser
    {
    public:
        explicit array_parser(parser& parser) : int_parser(static_cast<char>(data_type::Array)), m_parser(parser) {}

        [[nodiscard]] std::expected<size_t, std::error_code> parse(std::string_view value,
                                                                   std::vector<data>& data_) override;

    private:
        parser& m_parser;
    };

} // namespace LambdaSnail::resp::v2

profile_constexpr std::expected<size_t, std::error_code> LambdaSnail::resp::v2::parser::add_buffer(std::string_view buffer, std::vector<data>& data_)
{
    if (buffer.empty()) [[unlikely]]
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto it = buffer.begin();
    while (it != buffer.end() and not m_is_done)
    {
        if (m_parsers.empty())
        {
            auto ec = add_parser(it);
            if (ec)
            {
                return std::unexpected(ec);
            }
        }

        assert(not m_parsers.empty());

        auto const& current_parser  = m_parsers.top();
        auto const result           = current_parser->parse(std::string_view(it, buffer.end()), data_);
        if (not result.has_value())
        {
            return std::unexpected(result.error());
        }

        auto const num         = result.value();
        auto const all_parsed  = current_parser->is_done();
        if (all_parsed)
        {
            m_parsers.pop();
        }

        std::advance(it, num);

        m_is_done = data_.size() == m_num_elements;
    }

    return it - buffer.begin();
}

std::error_code LambdaSnail::resp::v2::parser::add_parser(std::string_view::const_iterator start)
{
    switch (static_cast<data_type>(*start))
    {
        case data_type::Integer:
            m_parsers.emplace(std::move(std::make_shared<int_parser>()));
            break;
        case data_type::SimpleString:
            m_parsers.emplace(std::move(std::make_shared<simple_string_parser>()));
            break;
        case data_type::Array:
            m_parsers.emplace(std::move(std::make_shared<array_parser>(*this)));
            break;
        case data_type::Boolean:
            m_parsers.emplace(std::move(std::make_shared<boolean_parser>()));
            break;
        case data_type::Double:
            m_parsers.emplace(std::move(std::make_shared<double_parser>()));
            break;
        case data_type::BulkString:
            m_parsers.emplace(std::move(std::make_shared<bulk_string_parser>()));
            break;
        case data_type::Null:
            m_parsers.emplace(std::move(std::make_shared<null_parser>()));
            break;
        default:
            return parse_errc::UnknownRespType;
    }

    return parse_errc::Success;
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::int_parser::parse(std::string_view value,
                                                                                std::vector<data>& data_)
{
    ZoneScoped;

    if (value.empty())
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto it_start = value.begin();
    if (*it_start == m_prefix)
    {
        ++it_start;

        // We only need to check for negativity when parsing the first part of an integer
        m_is_negative = *it_start == '-';
        if (m_is_negative)
        {
            ++it_start;
        }
    }

    auto it = it_start;
    for (; it < value.end(); ++it)
    {
        switch (*it)
        {
            case '\r':
                continue;
                break;
            case '\n':
                ++it; // Compensate for premature loop exit
                m_is_fully_parsed = true;
                goto fully_parsed;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                m_state = (m_state * 10) + (*it - '0');
                break;
            default:
                return std::unexpected(parse_errc::InvalidToken);
        }
    }

fully_parsed:
    if (m_is_fully_parsed)
    {
        data_.emplace_back(m_is_negative ? -m_state : m_state);
    }

    return it - value.begin();
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::double_parser::parse(std::string_view value,
                                                                                   std::vector<data>& data_)
{
    ZoneScoped;

    if (value.empty())
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto it_start = value.begin();
    if (*it_start == m_prefix)
    {
        ++it_start;

        // We only need to check for negativity when parsing the first part of an integer
        m_is_negative = *it_start == '-';
        if (m_is_negative)
        {
            ++it_start;
        }
    }

    auto it = it_start;
    if (not m_is_fraction)
    {
        for (; it != value.end(); ++it)
        {
            switch (*it)
            {
                case '\r':
                    continue;
                    break;
                case '\n':
                    ++it; // Compensate for premature loop exit
                    m_is_fully_parsed = true;
                    goto exit;
                case '.':
                case ',':
                    ++it;
                    m_is_fraction = true;
                    goto fraction;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    m_state = (m_state * 10.) + (*it - '0');
                    break;
                default:
                    return std::unexpected(parse_errc::InvalidToken);
            }
        }
    }

fraction:
    if (m_is_fraction)
    {
        for (; it != value.end(); ++it)
        {
            switch (*it)
            {
                case '\r':
                    continue;
                    break;
                case '\n':
                    ++it; // Compensate for premature loop exit
                    m_is_fully_parsed = true;
                    goto exit;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    m_fraction = m_fraction + (*it - '0') * std::pow(.1, m_power++);
                    break;
                default:
                    return std::unexpected(parse_errc::InvalidToken);
            }
        }
    }

exit:
    if (m_is_fully_parsed)
    {
        data_.emplace_back((m_state + m_fraction) * (m_is_negative ? -1. : 1.));
    }

    return it - value.begin();
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::boolean_parser::parse(std::string_view value,
                                                                                    std::vector<data>& data_)
{
    ZoneScoped;

    assert(not value.empty());

    auto start = value.begin();
    if (*start == m_prefix)
    {
        ++start;

        switch (*start)
        {
            case '1':
            case 't':
            case 'T':
                m_state = true;
                break;
            case '0':
            case 'f':
            case 'F':
                m_state = false;
                break;
            default:
                return std::unexpected(parse_errc::InvalidToken);
        }
    }

    // Now we simply need to find the end of the value
    // Note that for simplicity, this is very permissive and will pass many cases that probably shouldn't be allowed
    for (; start != value.end(); ++start)
    {
        if (*start == '\r')
        {
            continue;
        }

        if (*start == '\n')
        {
            ++start;
            m_is_fully_parsed = true;
            break;
        }
    }

    if (m_is_fully_parsed)
    {
        data_.emplace_back(m_state);
    }

    return start - value.begin();
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::null_parser::parse(std::string_view value,
                                                                                 std::vector<data>& data_)
{
    ZoneScoped;

    if (value.empty())
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto start = value.begin();
    if (*start == m_prefix)
    {
        ++start;
    }

    // Now we simply need to find the end of the value
    for (; start != value.end(); ++start)
    {
        if (*start == '\r')
        {
            continue;
        }

        if (*start == '\n')
        {
            ++start;
            m_is_fully_parsed = true;
            break;
        }

        // ANy other character is an error for this type
        return std::unexpected(parse_errc::InvalidToken);
    }

    if (m_is_fully_parsed)
    {
        data_.emplace_back(null{});
    }

    return start - value.begin();
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::simple_string_parser::parse(std::string_view value,
                                                                                          std::vector<data>& data_)
{
    ZoneScoped;

    // Empty strings are allowed, but empty (simple) strings in RESP should be defined with three characters
    if (value.empty())
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto start = value.begin();
    if (*start == m_prefix)
    {
        ++start;
    }

    int rn_adjustment = 0;
    auto it = start;
    for (; it != value.end(); ++it)
    {
        if (*it == '\r')
        {
            ++rn_adjustment;
            continue;
        }

        if (*it == '\n')
        {
            ++rn_adjustment;
            ++it;
            m_is_fully_parsed = true;
            break;
        }
    }

    auto const num_characters = it - start - rn_adjustment;
    m_state += value.substr(start - value.begin(), num_characters);

    if (m_is_fully_parsed)
    {
        data_.emplace_back(m_state);
    }

    return it - value.begin();
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::bulk_string_parser::parse(std::string_view value,
                                                                                        std::vector<data>& data_)
{
    ZoneScoped;

    if (value.empty())
    {
        return std::unexpected(parse_errc::EmptyString);
    }

    auto start = value.begin();

    if (not m_size_parser.is_done())
    {
        std::vector<data> size_v{};
        auto const result = m_size_parser.parse(value, size_v);
        if (not result.has_value())
        {
            return result;
        }

        auto const read = result.value();
        if (m_size_parser.is_done())
        {
            assert(size_v.size() == 1);
            m_size = std::get<int64_t>(size_v[0]);
            if (m_size > resp_traits::MaxStringSizeBytes)
            {
                return std::unexpected(parse_errc::PayloadTooLarge);
            }

            m_state.reserve(m_size);
        }

        // Fully read, no characters left in value, or
        // Not fully read, consumed all characters
        if (read == value.size())
        {
         return value.size();
        }

        // Fully read, characters left in value
        std::advance(start, read);
    }

    auto it = start;
    for (; it < value.end() and m_size > 0; ++it)
    {
        m_state.push_back(*it);
        --m_size;
    }

    //m_state += std::string_view(start, it);
    //std::copy(start, it, m_state.end());

    if (m_size > 0)
    {
        return it - value.begin();
    }

    if (m_parsed_line_ending > 0)
    {
        for (; it < value.end() and m_parsed_line_ending > 0; ++it)
        {
            if (*it == '\r' or *it == '\n') // Technically we also allow strings ending with \n\r ...
            {
                --m_parsed_line_ending;
            }
            else
            {
                return std::unexpected(parse_errc::InvalidToken);
            }
        }
    }

    if (m_parsed_line_ending == 0)
    {
        m_is_fully_parsed = true;
        data_.emplace_back(m_state);
    }

    return it - value.begin();
}

void LambdaSnail::resp::v2::parser::reset()
{
    m_num_elements = 1;
    m_is_done = false;
    while (not m_parsers.empty())
    {
        m_parsers.pop();
    }
}

std::expected<size_t, std::error_code> LambdaSnail::resp::v2::array_parser::parse(std::string_view value,
                                                                                  std::vector<data>& data_)
{
    ZoneScoped;

    std::vector<data> d{};
    auto const result = int_parser::parse(value, d);
    if (not result.has_value())
    {
        return result;
    }

    auto const num = result.value();
    if (is_done())
    {
        assert(not d.empty());

        int64_t const array_size = std::get<int64_t>(d[0]);
        assert(array_size > 0);

        data_.reserve(static_cast<size_t>(array_size));
        m_parser.set_expected_num_elements(array_size);
    }

    return num;
}