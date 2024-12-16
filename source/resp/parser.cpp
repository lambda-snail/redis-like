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

        // template<typename T, typename Tag>
        // T materialize(Tag tag) const;
        [[nodiscard]] bool materialize(Boolean) const;
        [[nodiscard]] int64_t materialize(Integer) const;
        [[nodiscard]] double_t materialize(Double) const;
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
    };
}


LambdaSnail::resp::data_view::data_view(std::string_view message)
{
    parser p;
    (*this) = p.parse_message_s(message);
}

LambdaSnail::resp::data_view::data_view(data_type type, std::string_view message) : type(type), value(message) { }

//template<>
bool LambdaSnail::resp::data_view::materialize(Boolean tag) const
{
    if (value.size() == 1) [[likely]]
    {
        switch(value[0])
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
    bool const is_negative { value.size() > 1 and value[0] == '-' };
    auto it_start = value.begin();
    if(is_negative)
    {
        ++it_start;
    }

    int64_t integer{};
    for(auto i = it_start; i < value.end(); ++i)
    {
        integer = (integer*10)+(*i - '0');
    }

    return is_negative ? -integer : integer;
}

double_t LambdaSnail::resp::data_view::materialize(Double) const
{
    bool const is_negative { value.size() > 1 and value[0] == '-' };
    auto it_start = value.begin();
    if(is_negative)
    {
        ++it_start;
    }

    double_t number{};
    auto i = it_start;
    for(; i < value.end() and (*i != '.' and *i != ','); ++i)
    {
        number = (number*10.)+(*i - '0');
    }

    double_t fraction{};
    for(auto j = value.end()-1; j > i; --j)
    {
        fraction = (fraction*.1)+(*j - '0');
    }

    return number + fraction*.1;
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

    data_view data { data_type::SimpleError, {} };
    for(auto i = start+1; i < message.cend(); ++i)
    {
        if(*i == '\n' and *(i-1) == '\r') [[unlikely]]
        {
            end = i-1;
            data.value = std::string_view(start+1, end);
            data.type = static_cast<data_type>(*start);
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
            return data;
        case data_type::SimpleError:
        default:
            break;
    }

    return { data_type::SimpleError, "Unable to parse string as a resp type" };
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_integral(data_view const data) const
{
    auto it_start = data.value.begin();
    if(data.value.size() > 1 and data.value[0] == '-')
    {
        ++it_start;
    }

    for(auto i = it_start; i < data.value.end(); ++i)
    {
        if(auto const c = *i; c < '0' or c > '9') [[unlikely]]
        {
            return { data_type::SimpleError, "Unable to parse string as a integral type" };
        }
    }

    return data;
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_double(data_view const data) const
{
    auto it_start = data.value.begin();
    if(data.value.size() > 1 and data.value[0] == '-')
    {
        ++it_start;
    }

    for(auto i = it_start; i < data.value.end(); ++i)
    {
        if(auto const c = *i; (c < '0' or c > '9') and c != '.' and c != ',') [[unlikely]]
        {
            return { data_type::SimpleError, "Unable to parse string as a double type" };
        }
    }

    return data;
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_boolean(data_view const data) const
{
    if (data.value.size() == 1) [[likely]]
    {
        switch(data.value[0])
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
    return data.value.empty() ? data : data_view{ data_type::SimpleError, "Unable to parse string as a null type" };
}
