module;

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

export module resp : iface.parser;

namespace LambdaSnail::resp
{
    export enum class data_type
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
        data_view() = default;
        data_view(data_type type, std::string_view message);
        explicit data_view(std::string_view message);

        data_type type{};
        std::string_view value;

        [[nodiscard]] bool is_null() const;
        [[nodiscard]] bool materialize(Boolean) const;
        [[nodiscard]] int64_t materialize(Integer) const;
        [[nodiscard]] double_t materialize(Double) const;

        [[nodiscard]] std::string_view materialize(SimpleString) const;
        [[nodiscard]] std::string_view materialize(BulkString) const;
        [[nodiscard]] std::vector<data_view> materialize(Array) const;
    };

    struct array
    {
        std::vector<data_view> value{};
    };

    export class parser
    {
    public:
        [[nodiscard]] data_view parse_message_s(std::string_view const& message) const;
        [[nodiscard]] data_view parse_message_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

        [[nodiscard]] data_view parse_array_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;
        [[nodiscard]] data_view parse_bulk_string_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

    private:
        [[nodiscard]] data_view find_end_s(std::string_view const &message) const;
        [[nodiscard]] data_view find_end_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

        [[nodiscard]] data_view validate_integral(data_view data) const;
        [[nodiscard]] data_view validate_double(data_view data) const;
        [[nodiscard]] data_view validate_boolean(data_view data) const;
        [[nodiscard]] data_view validate_null(data_view data) const;
        [[nodiscard]] data_view validate_simple_string(data_view data) const;
    };
}


LambdaSnail::resp::data_view::data_view(std::string_view message) //: value(message)
{
    //type = value.empty() ? data_type::Null : static_cast<data_type>(value[0]);
    parser p;
    (*this) = p.parse_message_s(message);
}

LambdaSnail::resp::data_view::data_view(data_type type, std::string_view message) : type(type), value(message) { }

bool LambdaSnail::resp::data_view::is_null() const
{
    return type == data_type::Null;
}

bool LambdaSnail::resp::data_view::materialize(Boolean tag) const
{
    bool const is_bool = not value.empty() && value[0] == '#';
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

int64_t LambdaSnail::resp::data_view::materialize(Integer) const
{
    auto it_start = value.begin();
    if(not value.empty() and *it_start == ':')
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

double_t LambdaSnail::resp::data_view::materialize(Double) const
{
    auto it = value.begin();
    if(not value.empty() and *it == ',')
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

    return number + fraction*.1;
}

std::string_view LambdaSnail::resp::data_view::materialize(SimpleString) const
{
    return value;
}

std::string_view LambdaSnail::resp::data_view::materialize(BulkString) const
{
    return value;
}

std::vector<LambdaSnail::resp::data_view> LambdaSnail::resp::data_view::materialize(Array) const
{
    auto cursor = value.begin();
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


LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message_s(std::string_view const &message) const
{
    auto dummy = message.begin();
    return parse_message_s(message, message.begin(), dummy);
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    switch(static_cast<data_type>(*start))
    {
        case data_type::Array:
            return parse_array_s(message, ++start, end);
        case data_type::BulkString:
            return parse_bulk_string_s(message, ++start, end);
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_array_s(std::string_view const& message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    if(start == end)
    {
        return { data_type::Array, {} };
    }

    auto cursor = start;
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_bulk_string_s(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator&end) const
{
    if(start == end)
    {
        return { data_type::BulkString, {} };
    }

    auto cursor = start;
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

    start = cursor;
    std::ranges::advance(cursor, static_cast<std::iter_difference_t<std::string_view::iterator>>(length));
    data_view const data { data_type::BulkString, std::string_view(start, cursor) };

    ++cursor;
    ++cursor;
    end = cursor;

    return data;
}
















LambdaSnail::resp::data_view LambdaSnail::resp::parser::find_end_s(std::string_view const& message) const
{
    std::string_view::iterator dummy;
    return find_end_s(message, message.cbegin(), dummy);
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::find_end_s(std::string_view const& message, std::string_view::iterator start, std::string_view::iterator &end) const
{
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_integral(data_view const data) const
{
    auto it_start = data.value.begin();
    if(not data.value.empty() and *it_start == ':')
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_double(data_view const data) const
{
    auto it_start = data.value.begin();
    if(not data.value.empty() and *it_start == ',')
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_boolean(data_view const data) const
{
    bool const is_bool = not data.value.empty() && data.value[0] == '#';
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

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_null(data_view const data) const
{
    auto const is_length_correct = data.value.size() == 1 or data.value.size() == 3;
    auto const is_type_correct = data.value[0] == '_';
    return is_length_correct and is_type_correct ? data : data_view{ data_type::SimpleError, "Unable to parse string as a null type" };
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_simple_string(data_view data) const
{
    auto it = data.value.end();
    if(data.value.size() >= 2 and *(--it) == '\n' and *(--it) == '\r')
    {
        return data;
    }

    return data_view{ data_type::SimpleError, "Unable to parse value as SimpleString" };
}
