module;

#include <cstdint>
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

    struct data_view
    {
        data_type type{};
        std::string_view value;
    };

    struct array
    {
        std::vector<data_view> value{};
    };

    export class parser
    {
    public:
        [[nodiscard]] data_view parse_message(std::string_view const& message) const;
        [[nodiscard]] data_view parse_message(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;

        [[nodiscard]] data_view parse_array(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;
        [[nodiscard]] data_view parse_bulk_string(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator &end) const;


    //     [[nodiscard]] data_view parse_simple(std::string const& message) const;
    //     [[nodiscard]] data_view parse_simple(std::string const& message, std::string::const_iterator start) const;
    //
    // private:
    //     [[nodiscard]] data_view validate_integral(data_view const data) const;
    //     [[nodiscard]] data_view validate_double(data_view const data) const;
    //     [[nodiscard]] data_view validate_boolean(data_view const data) const;
    //     [[nodiscard]] data_view validate_null(data_view const data) const;
    };
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message(std::string_view const &message) const
{
    auto dummy = message.begin();
    return parse_message(message, message.begin(), dummy);
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_message(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    switch(static_cast<data_type>(*start))
    {
        case data_type::Array:
            return parse_array(message, ++start, end);
        case data_type::BulkString:
            return parse_bulk_string(message, ++start, end);
    }
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_array(std::string_view const& message, std::string_view::iterator start, std::string_view::iterator& end) const
{
    if(start == end)
    {
        return { .type = data_type::Array, .value = {} };
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
        return { .type = data_type::Array, .value = {} };
    }

    ++cursor; // '\r'
    ++cursor; // '\n'

    //std::vector<data_view> values(length);
    for(size_t i = 0; i < length; ++i)
    {
        //values[i] = parse_message(message, cursor, end);
        auto next_string = parse_message(message, cursor, end);
        cursor = end;
    }

    return { .type = data_type::Array, .value = std::string_view(start, end) };
}

LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_bulk_string(std::string_view const &message, std::string_view::iterator start, std::string_view::iterator&end) const
{
    if(start == end)
    {
        return { .type = data_type::BulkString, .value = {} };
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
        return { .type = data_type::BulkString, .value = {} };
    }

    ++cursor; // '\r'
    ++cursor; // '\n'

    start = cursor;
    std::ranges::advance(cursor, static_cast<std::iter_difference_t<std::string_view::iterator>>(length));
    data_view const data { .type = data_type::BulkString, .value = std::string_view(start, cursor) };

    ++cursor;
    ++cursor;
    end = cursor;

    return data;
}
















// LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_simple(std::string const& message) const
// {
//     return parse_simple(message, message.cbegin());
// }
//
// LambdaSnail::resp::data_view LambdaSnail::resp::parser::parse_simple(std::string const& message, std::string::const_iterator start) const
// {
//     if(message.empty())
//     {
//         return { .type = data_type::SimpleError, .value = "Cannot parse empty string to a resp type" };
//     }
//
//     data_view data { .type = data_type::SimpleError, .value = {} };
//     for(auto i = start+1; i < message.cend(); ++i)
//     {
//         if(*i == '\n' and *(i-1) == '\r') [[unlikely]]
//         {
//             data.value = std::string_view(start+1, i-1);
//             data.type = static_cast<data_type>(*start);
//             break;
//         }
//     }
//
//     // TODO: This essentially does two passes on the data, should consolidate into one pass
//     switch(data.type)
//     {
//         case data_type::Integer:
//             return validate_integral(data);
//         case data_type::Double:
//             return validate_double(data);
//         case data_type::Boolean:
//             return validate_boolean(data);
//         case data_type::Null:
//             return validate_null(data);
//         case data_type::SimpleString:
//             return data;
//         case data_type::SimpleError:
//             break;
//     }
//
//     return { .type = data_type::SimpleError, .value = "Unable to parse string as a resp type" };
// }
//
// LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_integral(data_view const data) const
// {
//     auto it_start = data.value.begin();
//     if(data.value.size() > 1 and data.value[0] == '-')
//     {
//         ++it_start;
//     }
//
//     for(auto i = it_start; i < data.value.end(); ++i)
//     {
//         if(auto const c = *i; c < '0' or c > '9') [[unlikely]]
//         {
//             return { .type = data_type::SimpleError, .value = "Unable to parse string as a integral type" };
//         }
//     }
//
//     return data;
// }
//
// LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_double(data_view const data) const
// {
//     auto it_start = data.value.begin();
//     if(data.value.size() > 1 and data.value[0] == '-')
//     {
//         ++it_start;
//     }
//
//     for(auto i = it_start; i < data.value.end(); ++i)
//     {
//         if(auto const c = *i; (c < '0' or c > '9') and c != '.' and c != ',') [[unlikely]]
//         {
//             return { .type = data_type::SimpleError, .value = "Unable to parse string as a double type" };
//         }
//     }
//
//     return data;
// }
//
// LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_boolean(data_view const data) const
// {
//     if (data.value.size() == 1) [[likely]]
//     {
//         switch(data.value[0])
//         {
//             case 't':
//             case 'T':
//             case 'f':
//             case 'F':
//                 return data;
//         }
//     }
//
//     return { .type = data_type::SimpleError, .value = "Unable to parse string as a boolean type" };
// }
//
// LambdaSnail::resp::data_view LambdaSnail::resp::parser::validate_null(data_view const data) const
// {
//     return data.value.empty() ? data : (data_view{ .type = data_type::SimpleError, .value = "Unable to parse string as a null type" });
// }
