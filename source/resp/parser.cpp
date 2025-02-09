module;

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
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

export module resp :resp.parser;

namespace LambdaSnail::resp
{
    export enum class data_type : uint8_t
    {
        SimpleString    = '+',
        SimpleError     = '-',
        Integer         = ':',
        Boolean         = '#',
        Double          = ',',
        Null            = '_',
        Array           = '*',
        BulkString      = '$'
    };

    export struct Boolean {};
    export struct Double {};
    export struct Integer {};
    export struct Array {};
    export struct BulkString {};
    export struct SimpleString {};

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
        [[nodiscard]] profile_constexpr data_view parse_message_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

        [[nodiscard]] profile_constexpr data_view parse_array_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;
        [[nodiscard]] profile_constexpr data_view parse_bulk_string_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

    private:
        [[nodiscard]] profile_constexpr data_view find_end_s(std::string_view const &message) const;
        [[nodiscard]] profile_constexpr data_view find_end_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

        [[nodiscard]] profile_constexpr data_view validate_integral(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_double(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_boolean(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_null(data_view data) const;
        [[nodiscard]] profile_constexpr data_view validate_simple_string(data_view data) const;
    };

    export inline namespace literals
    {
        constexpr std::string resp_end = "\r\n";

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
}


profile_constexpr LambdaSnail::resp::data_view::data_view(std::string_view message)
{
    parser p;
    (*this) = p.parse_message_s(message);
}

profile_constexpr LambdaSnail::resp::data_view::data_view(data_type type, std::string_view message) : type(type), value(message) { }

profile_constexpr bool LambdaSnail::resp::data_view::is_null() const
{
    return type == data_type::Null;
}

profile_constexpr bool LambdaSnail::resp::data_view::materialize(Boolean tag) const
{
    ZoneScoped;

    bool const is_bool = not value.empty() && value[0] == static_cast<char>(data_type::Boolean);
    bool const has_correct_length = (value.size() == 2) or (value.size() == 4 and value[2] == '\r' and value[3] == '\n');

    if (is_bool and has_correct_length) [[likely]]
    {
        switch(value[1])
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
    if(not value.empty() and *it_start == static_cast<char>(data_type::Integer))
    {
        ++it_start;
    }

    bool const is_negative { value.size() > 1 and *it_start == '-' };
    if(is_negative)
    {
        ++it_start;
    }

    auto end = value.end();
    if(*(end-1) == '\n')
    {
        end -= 2;
    }

    int64_t integer{};
    for(auto i = it_start; i < end; ++i)
    {
        integer = (integer*10)+(*i - '0');
    }

    return is_negative ? -integer : integer;
}

profile_constexpr double_t LambdaSnail::resp::data_view::materialize(Double) const
{
    ZoneScoped;

    auto it = value.begin();
    if(not value.empty() and *it == static_cast<char>(data_type::Double))
    {
        ++it;
    }

    bool const is_negative { value.size() > 1 and *it == '-' };
    if(is_negative)
    {
        ++it;
    }

    auto end = value.end();
    if(*(end-1) == '\n')
    {
        end -= 2;
    }

    double_t number{};
    for(; it < end and (*it != '.' and *it != ','); ++it)
    {
        number = (number*10.)+(*it - '0');
    }

    double_t fraction{};
    for(auto j = end-1; j > it; --j)
    {
        fraction = (fraction*.1)+(*j - '0');
    }

    return (number + fraction*.1) * (is_negative ? -1 : 1);
}

profile_constexpr std::string_view LambdaSnail::resp::data_view::materialize(SimpleString) const
{
    ZoneScoped;

    size_t start = 0;
    size_t length = value.length();
    if(not value.empty() and *value.begin() == static_cast<char>(data_type::SimpleString)) [[likely]]
    {
        --length;
        ++start;
    }

    if(value.size() > 1 and *(value.end()-1) == '\n')
    {
        length -= 2;
    }

    return value.substr(start, length);
}

profile_constexpr std::string_view LambdaSnail::resp::data_view::materialize(BulkString) const
{
    ZoneScoped;

    assert(*value.begin() == static_cast<char>(data_type::BulkString));

    if(value.size() == 1)
    {
        return {};
    }

    auto cursor = value.begin() + 1;
    size_t length {0};

    while(*cursor != '\r')
    {
        length = (length*10)+(*cursor - '0');
        ++cursor;
    }

    if(not length) [[unlikely]]
    {
        return {};
    }

    // Skip the \r\n
    ++cursor;
    ++cursor;

    auto end = cursor;
    std::advance(end, static_cast<std::iter_difference_t<std::string_view>>(length));

    return { cursor, end };
}

profile_constexpr std::vector<LambdaSnail::resp::data_view> LambdaSnail::resp::data_view::materialize(Array) const
{
    ZoneScoped;

    assert(*value.begin() == static_cast<char>(data_type::Array));

    auto cursor = value.begin() + 1;
    size_t length {0};

    while(*cursor != '\r')
    {
        length = (length*10)+(*cursor - '0');
        ++cursor;
    }

    if(not length) [[unlikely]]
    {
        return {};
    }

    ++cursor;
    ++cursor;

    parser constexpr p;
    std::vector<data_view> values(length);
    for(size_t i = 0; i < length; ++i)
    {
        std::string_view::iterator end;
        values[i] = p.parse_message_s(value, cursor, end);
        cursor = end;
    }

    return values;
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message_s(std::string_view const &message) const
{
    ZoneScoped;

    auto dummy = message.end();
    return parse_message_s(message, message.begin(), dummy);
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    ZoneScoped;

    switch(static_cast<data_type>(*start))
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
            return find_end_s(message, start, end); // How do we communicate that we need start and not ++start from an api perspective?
        default:
            break;
    }

    return { data_type::SimpleError, "Unsupported type: " + *start };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_array_s(std::string_view const& message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    ZoneScoped;

    if(start == end or (message.size() == 1 and *start == static_cast<char>(data_type::Array)))
    {
        return { data_type::Array, {} };
    }

    auto cursor = start + 1;
    size_t length {0};

    while(*cursor != '\r')
    {
        length = (length*10)+(*cursor - '0');
        ++cursor;
    }

    if(not length) [[unlikely]]
    {
        return { data_type::Array, {} };
    }

    ++cursor; // '\r'
    ++cursor; // '\n'
    for(size_t i = 0; i < length; ++i)
    {
        //values[i] = parse_message(message, cursor, end);
        auto next_string = parse_message_s(message, cursor, end);
        cursor = end;

        if(next_string.type == data_type::SimpleError)
        {
            return next_string;
        }
    }

    return { data_type::Array, std::string_view(start, end) };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_bulk_string_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator&end) const
{
    ZoneScoped;

    if(message.size() == 1)
    {
        return { data_type::BulkString, {} };
    }

    auto cursor = start + 1;
    size_t length {0};

    while(*cursor != '\r')
    {
        length = (length*10)+(*cursor - '0');
        ++cursor;
    }

    if(not length) [[unlikely]]
    {
        return { data_type::BulkString, {} };
    }

    ++cursor; // '\r'
    ++cursor; // '\n'

    //start = cursor;
    std::ranges::advance(cursor, static_cast<std::iter_difference_t<std::string_view::iterator>>(length));
    data_view const data { data_type::BulkString, std::string_view(start, cursor) };

    ++cursor;
    ++cursor;
    end = cursor;

    return data;
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::find_end_s(std::string_view const& message) const
{
    ZoneScoped;

    std::string_view::iterator dummy;
    return find_end_s(message, message.cbegin(), dummy);
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::find_end_s(std::string_view const& message, std::string_view::iterator start, std::string_view::iterator &end) const
{
    ZoneScoped;

    if(message.empty())
    {
        return { data_type::SimpleError, "Cannot parse empty string to a resp type" };
    }

    data_view data { static_cast<data_type>(*start), {} };
    for(auto i = start+1; i < message.cend(); ++i)
    {
        if(*i == '\n' and *(i-1) == '\r') [[unlikely]]
        {
            end = i+1; // one past the ending
            data.value = std::string_view(start, end);
            //data.type = static_cast<data_type>(*start);
            break;
        }
    }

    // TODO: Do we validate here or when creating the actual data?
    switch(data.type)
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

    return { data_type::SimpleError, "Unable to parse string as a resp type" };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_integral(data_view const data) const
{
    ZoneScoped;

    auto it_start = data.value.begin();
    if(not data.value.empty() and *it_start == static_cast<char>(data_type::Integer))
    {
        ++it_start;
    }

    bool const is_negative { data.value.size() > 1 and *it_start == '-' };
    if(is_negative)
    {
        ++it_start;
    }

    auto i = it_start;
    for(; i < data.value.end(); ++i)
    {
        if(auto const c = *i; c < '0' or c > '9') [[unlikely]]
        {
            break;
        }
    }

    if(data.value.end()-i == 2)
    {
        return data;
    }

    return { data_type::SimpleError, "Unable to parse string as an integer type" };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_double(data_view const data) const
{
    ZoneScoped;

    auto it_start = data.value.begin();
    if(not data.value.empty() and *it_start == static_cast<char>(data_type::Double))
    {
        ++it_start;
    }

    bool const is_negative { data.value.size() > 1 and *it_start == '-' };
    if(is_negative)
    {
        ++it_start;
    }

    auto i = it_start;
    for(; i < data.value.end(); ++i)
    {
        if(auto const c = *i; (c < '0' or c > '9') and c != '.' and c != ',') [[unlikely]]
        {
            break;
        }
    }

    if(data.value.end()-i == 2)
    {
        return data;
    }

    return { data_type::SimpleError, "Unable to parse string as a double type" };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_boolean(data_view const data) const
{
    ZoneScoped;

    bool const is_bool = not data.value.empty() && data.value[0] == static_cast<char>(data_type::Boolean);
    bool const has_correct_length = (data.value.size() == 2) or (data.value.size() == 4 and data.value[2] == '\r' and data.value[3] == '\n');

    if (is_bool and has_correct_length) [[likely]]
    {
        switch(data.value[1])
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

    return { data_type::SimpleError, "Unable to parse string as a boolean type" };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_null(data_view const data) const
{
    ZoneScoped;

    auto const is_length_correct = data.value.size() == 1 or data.value.size() == 3;
    auto const is_type_correct = data.value[0] == static_cast<char>(data_type::Null);
    return is_length_correct and is_type_correct ? data : data_view{ data_type::SimpleError, "Unable to parse string as a null type" };
}

profile_constexpr LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_simple_string(data_view data) const
{
    ZoneScoped;

    auto it = data.value.end();
    if(data.value.size() >= 2 and data.value[0] == static_cast<char>(data_type::SimpleString) and *(--it) == '\n' and *(--it) == '\r')
    {
        return data;
    }

    return data_view{ data_type::SimpleError, "Unable to parse value as SimpleString" };
}
