module;

#include <cstdint>
#include <span>
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

    struct simple_string_data
    {
        data_type type{};
        std::string_view value{};
    };

    struct command
    {
        std::vector<simple_string_data> value{};
    };

    export class parser
    {
    public:
        [[nodiscard]] command parse(std::string_view const& message) const;

        [[nodiscard]] simple_string_data parse_simple(std::string const& message) const;
        [[nodiscard]] simple_string_data parse_simple(std::string const& message, std::string::const_iterator start) const;

    private:
        [[nodiscard]] simple_string_data validate_integral(simple_string_data const data) const;
        [[nodiscard]] simple_string_data validate_double(simple_string_data const data) const;
        [[nodiscard]] simple_string_data validate_boolean(simple_string_data const data) const;
        [[nodiscard]] simple_string_data validate_null(simple_string_data const data) const;
    };
}

LambdaSnail::resp::command LambdaSnail::resp::parser::parse(std::string_view const& message) const
{
    command command;
    if(message.empty())
    {
        return command;
    }

    auto cursor = message.begin();
    switch (*cursor)
    {
        case '*':
            break;
        case '$':
            break;
    }

    return command;
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::parse_simple(std::string const& message) const
{
    return parse_simple(message, message.cbegin());
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::parse_simple(std::string const& message, std::string::const_iterator start) const
{
    if(message.empty())
    {
        return { .type = data_type::SimpleError, .value = "Cannot parse empty string to a resp type" };
    }

    simple_string_data data;
    data.type = data_type::SimpleError;
    for(auto i = start+1; i < message.cend(); ++i)
    {
        if(*i == '\n' and *(i-1) == '\r') [[unlikely]]
        {
            data.value = std::string_view(start+1, i-1);
            data.type = static_cast<data_type>(*start);
            break;
        }
    }

    // TODO: This essentially does two passes on the data, should consolidate into one pass
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
            break;
    }

    return { .type = data_type::SimpleError, .value = "Unable to parse string as a resp type" };
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::validate_integral(simple_string_data const data) const
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
            return { .type = data_type::SimpleError, .value = "Unable to parse string as a integral type" };
        }
    }

    return data;
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::validate_double(simple_string_data const data) const
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
            return { .type = data_type::SimpleError, .value = "Unable to parse string as a double type" };
        }
    }

    return data;
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::validate_boolean(simple_string_data const data) const
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
        }
    }

    return { .type = data_type::SimpleError, .value = "Unable to parse string as a boolean type" };
}

LambdaSnail::resp::simple_string_data LambdaSnail::resp::parser::validate_null(simple_string_data const data) const
{
    return data.value.empty() ? data : (simple_string_data{ .type = data_type::SimpleError, .value = "Unable to parse string as a null type" });
}
