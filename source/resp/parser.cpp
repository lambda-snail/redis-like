module;

#include <asio/detail/reactive_socket_accept_op.hpp>
#include <cassert>
#include <cmath>
#include <expected>
#include <stack>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <tracy/Tracy.hpp>

#include "../../build/debug/cli11_proj-src/include/CLI/TypeTools.hpp"

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

    export struct Boolean
    {
    };
    export struct Double
    {
    };
    export struct Integer
    {
    };
    export struct Array
    {
    };
    export struct BulkString
    {
    };
    export struct SimpleString
    {
    };

    export struct data_view
    {
        profile_constexpr data_view() = default;
        profile_constexpr data_view(data_type type, std::string_view message);
        profile_constexpr explicit data_view(std::string_view message);

        data_type type{};
        std::string_view value{};

        [[nodiscard]] profile_constexpr bool is_null() const;
        [[nodiscard]] profile_constexpr bool materialize(Boolean) const;
        [[nodiscard]] profile_constexpr int64_t materialize(Integer) const;
        [[nodiscard]] profile_constexpr double_t materialize(Double) const;

        [[nodiscard]] profile_constexpr std::string_view materialize(SimpleString) const;
        [[nodiscard]] profile_constexpr std::string_view materialize(BulkString) const;
        [[nodiscard]] profile_constexpr std::vector<data_view> materialize(Array) const;
    };

    class parser
    {
    public:
        [[nodiscard]] profile_constexpr data_view parse_message_s(std::string_view const& message) const;
        [[nodiscard]] profile_constexpr data_view parse_message_s(std::string_view const& message,
                                                                  std::string_view::iterator start,
                                                                  std::string_view::iterator& end) const;

        [[nodiscard]] profile_constexpr data_view parse_array_s(std::string_view const& message,
                                                                std::string_view::iterator start,
                                                                std::string_view::iterator& end) const;
        [[nodiscard]] profile_constexpr data_view parse_bulk_string_s(std::string_view const& message,
                                                                      std::string_view::iterator start,
                                                                      std::string_view::iterator& end) const;

    private:
        [[nodiscard]] profile_constexpr data_view find_end_s(std::string_view const& message) const;
        [[nodiscard]] profile_constexpr data_view find_end_s(std::string_view const& message,
                                                             std::string_view::iterator start,
                                                             std::string_view::iterator& end) const;

        [[nodiscard]] profile_constexpr data_view validate_integral(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_double(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_boolean(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_null(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_simple_string(data_view data) const;
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

    struct resp_error
    {
    };
} // namespace LambdaSnail::resp

namespace LambdaSnail::resp::v2
{
    export typedef std::variant<int64_t, std::string, double> data;

    class stateful_parser
    {
    public:
        explicit stateful_parser(char const prefix) : prefix_(prefix) {}

        //[[nodiscard]] virtual data get_value() const = 0;
        [[nodiscard]] virtual bool is_done() const { return is_fully_parsed; }
        [[nodiscard]] virtual size_t parse(std::string_view value, std::vector<data>& data_) = 0;

        virtual ~stateful_parser() = default;

    protected:
        char const prefix_;
        bool is_fully_parsed { false };
    };

    class int_parser : public stateful_parser
    {
    public:
        explicit int_parser(char const prefix = static_cast<char>(data_type::Integer)) : stateful_parser(prefix) {}

        //[[nodiscard]] data get_value() const override { assert(is_fully_parsed); return data{ state }; }
        [[nodiscard]] size_t parse(std::string_view value, std::vector<data>& data_) override;

        // int_parser(int_parser&& parser) noexcept = delete;
        // int_parser(const int_parser& parser) = delete;
        // int_parser& operator=(int_parser const&) const = delete;
        // int_parser& operator=(int_parser const&&) = delete;
    private:
        int64_t state {}; // Intermediate or fully parsed value
        bool is_negative { false };
    };

    class double_parser : public stateful_parser
    {
    public:
        explicit double_parser() : stateful_parser(static_cast<char>(data_type::Double)) {}

        [[nodiscard]] size_t parse(std::string_view value, std::vector<data>& data_) override;

    private:
        double state {};
        double fraction {};
        double power { 1 };
        bool is_fraction { false };
        bool is_negative { false };
    };

    class simple_string_parser final : public stateful_parser
    {
    public:
        explicit simple_string_parser() : stateful_parser(static_cast<char>(data_type::SimpleString)) {}

        [[nodiscard]] size_t parse(std::string_view value, std::vector<data>& data_) override;

    private:
        std::string state {};
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
    export class parser
    {
    public:
        [[nodiscard]] profile_constexpr size_t add_buffer(std::string_view buffer, std::vector<data>& data_);

        [[nodiscard]] inline bool is_done() const { return is_done_; };

        void set_num_elements(size_t num) { num_elements = num; };

    private:
        struct parse_result
        {
            bool is_done{false};
            size_t num_read{0};
        };

        size_t num_elements { 1 };
        bool is_done_ { false };

        std::stack<std::shared_ptr<stateful_parser>> parsers {};
        //std::shared_ptr<stateful_parser> current_parser {};
        void add_parser(std::string_view::const_iterator start);
    };

    /**
     * The array parser will always run first in a well-formed message. To make this work, the array parser
     * communicates to the parser how many elements are expected to appear in the message. This is slightly
     * convoluted but works for the purposes of this limited scenario where we know that there first thing
     * to parse is always an array, and no nested arrays exist.
     */
    class array_parser final : public int_parser
    {
    public:
        explicit array_parser(parser& parser) : int_parser(static_cast<char>(data_type::Array)), parser_(parser) {}

        [[nodiscard]] size_t parse(std::string_view value, std::vector<data>& data_) override;

    private:
        parser& parser_;
    };

} // namespace LambdaSnail::resp::v2

profile_constexpr size_t LambdaSnail::resp::v2::parser::add_buffer(std::string_view buffer, std::vector<data>& data_)
{
    if (buffer.empty()) [[unlikely]]
    {
        return 0;
    }

    auto it = buffer.begin();
    while (it != buffer.end())
    {
        if (parsers.empty())
        {
            add_parser(it);
        }

        assert(not parsers.empty());

        auto const& current_parser  = parsers.top();
        auto const num              = current_parser->parse(std::string_view(it, buffer.end()), data_);
        auto const all_parsed       = current_parser->is_done();
        if (all_parsed)
        {
            parsers.pop();
        }

        std::advance(it, num);
    }

    is_done_ = data_.size() == num_elements;

    return it - buffer.begin();
}

void LambdaSnail::resp::v2::parser::add_parser(std::string_view::const_iterator start)
{
    switch (static_cast<data_type>(*start))
    {
        case data_type::Integer:
            parsers.emplace(std::move(std::make_shared<int_parser>()));
            break;
        case data_type::SimpleString:
            parsers.emplace(std::move(std::make_shared<simple_string_parser>()));
            break;
        case data_type::Array:
            parsers.emplace(std::move(std::make_shared<array_parser>(*this)));
            break;
            // case data_type::BulkString:
            // case data_type::Boolean:
            case data_type::Double:
            parsers.emplace(std::move(std::make_shared<double_parser>()));
            break;

            // case data_type::Null:

        default:
            std::unreachable();
    }
}

size_t LambdaSnail::resp::v2::int_parser::parse(std::string_view value, std::vector<data>& data_)
{
    ZoneScoped;

    assert(not value.empty());

    auto it_start = value.begin();
    if (*it_start == prefix_)
    {
        ++it_start;

        // We only need to check for negativity when parsing the first part of an integer
        is_negative = *it_start == '-';
        if (is_negative)
        {
            ++it_start;
        }
    }

    auto it = it_start;
    for (; it < value.end(); ++it)
    {
        // TODO: Check for errors
        // if (*i < '0' or *i > '9')
        // {
        //     return error
        // }

        if (*it == '\r')
        {
            continue;
        }

        if (*it == '\n')
        {
            ++it; // Compensate for premature loop exit
            is_fully_parsed = true;
            break;
        }

        state = (state * 10) + (*it - '0');
    }

    if (is_fully_parsed)
    {
        data_.emplace_back(is_negative ? -state : state);
    }

    return it - value.begin();
}

size_t LambdaSnail::resp::v2::double_parser::parse(std::string_view value, std::vector<data>& data_)
{
    ZoneScoped;

    assert(not value.empty());

    auto it_start = value.begin();
    if (*it_start == prefix_)
    {
        ++it_start;

        // We only need to check for negativity when parsing the first part of an integer
        is_negative = *it_start == '-';
        if (is_negative)
        {
            ++it_start;
        }
    }

    auto it = it_start;
    if (not is_fraction)
    {
        for (; it != value.end(); ++it)
        {
            // TODO: Check for errors
            // if (*i < '0' or *i > '9')
            // {
            //     return error
            // }

            switch (*it)
            {
                case '\r':
                    continue;

                case '\n':
                    ++it; // Compensate for premature loop exit
                    is_fully_parsed = true;
                    goto exit;

                case '.':
                case ',':
                    ++it;
                    is_fraction = true;
                    goto fraction;

                default:
                    // TODO: Error handling
            }

            state = (state * 10.) + (*it - '0');
        }
    }

fraction:
    if (is_fraction)
    {
        // TODO: Check for errors
        for (; it != value.end(); ++it)
        {
            switch (*it)
            {
                case '\r':
                    continue;
                    break;

                case '\n':
                    ++it; // Compensate for premature loop exit
                    is_fully_parsed = true;
                    goto exit;
                    break;

                default:
                    // TODO: Error handling
            }

            fraction = fraction + (*it - '0') * std::pow(.1, power++);
        }
    }

exit:
    if (is_fully_parsed)
    {
        data_.emplace_back((state + fraction) * (is_negative ? -1. : 1.));
    }

    return it - value.begin();
}

size_t LambdaSnail::resp::v2::simple_string_parser::parse(std::string_view value, std::vector<data>& data_)
{
    ZoneScoped;

    assert(not value.empty());

    auto start = value.begin();
    if (*start == prefix_)
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
            is_fully_parsed = true;
            break;
        }
    }

    auto const num_characters = it - start - rn_adjustment;
    state += value.substr(start - value.begin(), num_characters);

    if (is_fully_parsed)
    {
        data_.emplace_back(state);
    }

    return it - value.begin();
}

size_t LambdaSnail::resp::v2::array_parser::parse(std::string_view value, std::vector<data>& data_)
{
    ZoneScoped;

    std::vector<data> d{};
    auto const num = int_parser::parse(value, d);

    if (is_done())
    {
        assert(not d.empty());

        int64_t const array_size = std::get<int64_t>(d[0]);
        assert(array_size > 0);

        data_.reserve(static_cast<size_t>(array_size));
        parser_.set_num_elements(array_size);
    }

    return num;
}























profile_constexpr LambdaSnail::resp::data_view::data_view(std::string_view message)
{
    parser p;
    (*this) = p.parse_message_s(message);
}

profile_constexpr LambdaSnail::resp::data_view::data_view(data_type type, std::string_view message) :
    type(type), value(message)
{
}

profile_constexpr bool LambdaSnail::resp::data_view::is_null() const { return type == data_type::Null; }

profile_constexpr bool LambdaSnail::resp::data_view::materialize(Boolean tag) const
{
    ZoneScoped;

    bool const is_bool = not value.empty() && value[0] == static_cast<char>(data_type::Boolean);
    bool const has_correct_length =
            (value.size() == 2) or (value.size() == 4 and value[2] == '\r' and value[3] == '\n');

    if (is_bool and has_correct_length) [[likely]]
    {
        switch (value[1])
        {
            case 't':
            case 'T':
                return true;
            case 'f':
            case 'F':
                return false;
            default:
                break;
        }
    }

    throw std::runtime_error("Attempt to materialize an invalid bool");
}

profile_constexpr int64_t LambdaSnail::resp::data_view::materialize(Integer) const
{
    ZoneScoped;

    auto it_start = value.begin();
    if (not value.empty() and *it_start == static_cast<char>(data_type::Integer))
    {
        ++it_start;
    }

    bool const is_negative{value.size() > 1 and *it_start == '-'};
    if (is_negative)
    {
        ++it_start;
    }

    auto end = value.end();
    if (*(end - 1) == '\n')
    {
        end -= 2;
    }

    int64_t integer{};
    for (auto i = it_start; i < end; ++i)
    {
        integer = (integer * 10) + (*i - '0');
    }

    return is_negative ? -integer : integer;
}

profile_constexpr double_t LambdaSnail::resp::data_view::materialize(Double) const
{
    ZoneScoped;

    auto it = value.begin();
    if (not value.empty() and *it == static_cast<char>(data_type::Double))
    {
        ++it;
    }

    bool const is_negative{value.size() > 1 and *it == '-'};
    if (is_negative)
    {
        ++it;
    }

    auto end = value.end();
    if (*(end - 1) == '\n')
    {
        end -= 2;
    }

    double_t number{};
    for (; it < end and (*it != '.' and *it != ','); ++it)
    {
        number = (number * 10.) + (*it - '0');
    }

    double_t fraction{};
    for (auto j = end - 1; j > it; --j)
    {
        fraction = (fraction * .1) + (*j - '0');
    }

    return (number + fraction * .1) * (is_negative ? -1 : 1);
}

profile_constexpr std::string_view LambdaSnail::resp::data_view::materialize(SimpleString) const
{
    ZoneScoped;

    size_t start  = 0;
    size_t length = value.length();
    if (not value.empty() and *value.begin() == static_cast<char>(data_type::SimpleString)) [[likely]]
    {
        --length;
        ++start;
    }

    if (value.size() > 1 and *(value.end() - 1) == '\n')
    {
        length -= 2;
    }

    return value.substr(start, length);
}

profile_constexpr std::string_view LambdaSnail::resp::data_view::materialize(BulkString) const
{
    ZoneScoped;

    assert(*value.begin() == static_cast<char>(data_type::BulkString));

    if (value.size() == 1)
    {
        return {};
    }

    auto cursor = value.begin() + 1;
    size_t length{0};

    while (*cursor != '\r')
    {
        length = (length * 10) + (*cursor - '0');
        ++cursor;
    }

    if (not length) [[unlikely]]
    {
        return {};
    }

    // Skip the \r\n
    ++cursor;
    ++cursor;

    auto end = cursor;
    std::advance(end, static_cast<std::iter_difference_t<std::string_view>>(length));

    return {cursor, end};
}

profile_constexpr std::vector<LambdaSnail::resp::data_view> LambdaSnail::resp::data_view::materialize(Array) const
{
    ZoneScoped;

    assert(*value.begin() == static_cast<char>(data_type::Array));

    auto cursor = value.begin() + 1;
    size_t length{0};

    while (*cursor != '\r')
    {
        length = (length * 10) + (*cursor - '0');
        ++cursor;
    }

    if (not length) [[unlikely]]
    {
        return {};
    }

    ++cursor;
    ++cursor;

    parser constexpr p;
    std::vector<data_view> values(length);
    for (size_t i = 0; i < length; ++i)
    {
        std::string_view::iterator end;
        values[i] = p.parse_message_s(value, cursor, end);
        cursor    = end;
    }

    return values;
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::parse_message_s(std::string_view const& message) const
{
    ZoneScoped;

    auto dummy = message.end();
    return parse_message_s(message, message.begin(), dummy);
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::parse_message_s(std::string_view const& message, std::string_view::iterator start,
                                           std::string_view::iterator& end) const
{
    ZoneScoped;

    switch (static_cast<data_type>(*start))
    {
        case data_type::Array:
            return parse_array_s(message, start, end);
        case data_type::BulkString:
            return parse_bulk_string_s(message, start, end);
        case data_type::Boolean:
        case data_type::Integer:
        case data_type::Double:
        case data_type::Null:
        case data_type::SimpleString:
            return find_end_s(message, start,
                              end); // How do we communicate that we need start and not ++start from an api perspective?
        default:
            break;
    }

    return {data_type::SimpleError, "Unsupported type: " + *start};
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::parse_array_s(std::string_view const& message, std::string_view::iterator start,
                                         std::string_view::iterator& end) const
{
    ZoneScoped;

    if (start == end or (message.size() == 1 and *start == static_cast<char>(data_type::Array)))
    {
        return {data_type::Array, {}};
    }

    auto cursor = start + 1;
    size_t length{0};

    while (*cursor != '\r')
    {
        length = (length * 10) + (*cursor - '0');
        ++cursor;
    }

    if (not length) [[unlikely]]
    {
        return {data_type::Array, {}};
    }

    ++cursor; // '\r'
    ++cursor; // '\n'
    for (size_t i = 0; i < length; ++i)
    {
        // values[i] = parse_message(message, cursor, end);
        auto next_string = parse_message_s(message, cursor, end);
        cursor           = end;

        if (next_string.type == data_type::SimpleError)
        {
            return next_string;
        }
    }

    return {data_type::Array, std::string_view(start, end)};
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::parse_bulk_string_s(std::string_view const& message, std::string_view::iterator start,
                                               std::string_view::iterator& end) const
{
    ZoneScoped;

    if (message.size() == 1)
    {
        return {data_type::BulkString, {}};
    }

    auto cursor = start + 1;
    size_t length{0};

    while (*cursor != '\r')
    {
        length = (length * 10) + (*cursor - '0');
        ++cursor;
    }

    if (not length) [[unlikely]]
    {
        return {data_type::BulkString, {}};
    }

    ++cursor; // '\r'
    ++cursor; // '\n'

    // start = cursor;
    std::ranges::advance(cursor, static_cast<std::iter_difference_t<std::string_view::iterator>>(length));
    data_view const data{data_type::BulkString, std::string_view(start, cursor)};

    ++cursor;
    ++cursor;
    end = cursor;

    return data;
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::find_end_s(std::string_view const& message) const
{
    ZoneScoped;

    std::string_view::iterator dummy;
    return find_end_s(message, message.cbegin(), dummy);
}

profile_constexpr LambdaSnail::resp::data_view
LambdaSnail::resp::parser::find_end_s(std::string_view const& message, std::string_view::iterator start,
                                      std::string_view::iterator& end) const
{
    ZoneScoped;

    if (message.empty())
    {
        return {data_type::SimpleError, "Cannot parse empty string to a resp type"};
    }

    data_view data{static_cast<data_type>(*start), {}};
    for (auto i = start + 1; i < message.cend(); ++i)
    {
        if (*i == '\n' and *(i - 1) == '\r') [[unlikely]]
        {
            end        = i + 1; // one past the ending
            data.value = std::string_view(start, end);
            // data.type = static_cast<data_type>(*start);
            break;
        }
    }

    // TODO: Do we validate here or when creating the actual data?
    switch (data.type)
    {
        case data_type::Integer:
            return validate_integral(data);
        case data_type::Double:
            return validate_double(data);
        case data_type::Boolean:
            return validate_boolean(data);
        case data_type::Null:
            return validate_null(data);
        case data_type::SimpleString:
            return validate_simple_string(data);
        case data_type::SimpleError:
        default:
            break;
    }

    return {data_type::SimpleError, "Unable to parse string as a resp type"};
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_integral(data_view const data) const
{
    ZoneScoped;

    auto it_start = data.value.begin();
    if (not data.value.empty() and *it_start == static_cast<char>(data_type::Integer))
    {
        ++it_start;
    }

    bool const is_negative{data.value.size() > 1 and *it_start == '-'};
    if (is_negative)
    {
        ++it_start;
    }

    auto i = it_start;
    for (; i < data.value.end(); ++i)
    {
        if (auto const c = *i; c < '0' or c > '9') [[unlikely]]
        {
            break;
        }
    }

    if (data.value.end() - i == 2)
    {
        return data;
    }

    return {data_type::SimpleError, "Unable to parse string as an integer type"};
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_double(data_view const data) const
{
    ZoneScoped;

    auto it_start = data.value.begin();
    if (not data.value.empty() and *it_start == static_cast<char>(data_type::Double))
    {
        ++it_start;
    }

    bool const is_negative{data.value.size() > 1 and *it_start == '-'};
    if (is_negative)
    {
        ++it_start;
    }

    auto i = it_start;
    for (; i < data.value.end(); ++i)
    {
        if (auto const c = *i; (c < '0' or c > '9') and c != '.' and c != ',') [[unlikely]]
        {
            break;
        }
    }

    if (data.value.end() - i == 2)
    {
        return data;
    }

    return {data_type::SimpleError, "Unable to parse string as a double type"};
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_boolean(data_view const data) const
{
    ZoneScoped;

    bool const is_bool = not data.value.empty() && data.value[0] == static_cast<char>(data_type::Boolean);
    bool const has_correct_length =
            (data.value.size() == 2) or (data.value.size() == 4 and data.value[2] == '\r' and data.value[3] == '\n');

    if (is_bool and has_correct_length) [[likely]]
    {
        switch (data.value[1])
        {
            case 't':
            case 'T':
            case 'f':
            case 'F':
                return data;
            default:
                break;
        }
    }

    return {data_type::SimpleError, "Unable to parse string as a boolean type"};
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_null(data_view const data) const
{
    ZoneScoped;

    auto const is_length_correct = data.value.size() == 1 or data.value.size() == 3;
    auto const is_type_correct   = data.value[0] == static_cast<char>(data_type::Null);
    return is_length_correct and is_type_correct
                   ? data
                   : data_view{data_type::SimpleError, "Unable to parse string as a null type"};
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_simple_string(data_view data) const
{
    ZoneScoped;

    auto it = data.value.end();
    if (data.value.size() >= 2 and data.value[0] == static_cast<char>(data_type::SimpleString) and *(--it) == '\n' and
        *(--it) == '\r')
    {
        return data;
    }

    return data_view{data_type::SimpleError, "Unable to parse value as SimpleString"};
}