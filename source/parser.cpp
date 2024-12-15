module;

#include <cstdint>
#include <span>
#include <string>

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
        Null            = '_'
    };

    struct string_data
    {
        data_type type{};
        std::string_view value{};
    };

    export class parser
    {
    public:
         [[nodiscard]] string_data parse(std::string const& message) const;
         [[nodiscard]] string_data parse(std::string const& message, std::string::const_iterator start) const;

    private:
        [[nodiscard]] string_data validate_integral(string_data const data) const;
        [[nodiscard]] string_data validate_double(string_data const data) const;
        [[nodiscard]] string_data validate_boolean(string_data const data) const;
        [[nodiscard]] string_data validate_null(string_data const data) const;
    };
}

LambdaSnail::resp::string_data LambdaSnail::resp::parser::parse(std::string const& message) const
{
    return parse(message, message.cbegin());
}

LambdaSnail::resp::string_data LambdaSnail::resp::parser::parse(std::string const& message, std::string::const_iterator start) const
{
    if(message.empty())
    {
        return { .type = data_type::SimpleError, .value = "Cannot parse empty string to a resp type" };
    }

    string_data data;
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

LambdaSnail::resp::string_data LambdaSnail::resp::parser::validate_integral(string_data const data) const
{
    for(auto const c : data.value)
    {
        if(c < '0' or c > '9') [[unlikely]]
        {
            return { .type = data_type::SimpleError, .value = "Unable to parse string as a integral type" };
        }
    }

    return data;
}

LambdaSnail::resp::string_data LambdaSnail::resp::parser::validate_double(string_data const data) const
{
    for(auto const c : data.value)
    {
        if((c < '0' or c > '9') and c != '.' and c != ',') [[unlikely]]
        {
            return { .type = data_type::SimpleError, .value = "Unable to parse string as a double type" };
        }
    }

    return data;
}

LambdaSnail::resp::string_data LambdaSnail::resp::parser::validate_boolean(string_data const data) const
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

LambdaSnail::resp::string_data LambdaSnail::resp::parser::validate_null(string_data const data) const
{
    return data.value.empty() ? data : (string_data{ .type = data_type::SimpleError, .value = "Unable to parse string as a null type" });
}
