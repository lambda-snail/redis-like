import resp;

#include <gtest/gtest.h>

struct ParserTypeTest : public testing::TestWithParam<std::tuple<char const*, LambdaSnail::resp::data_type>>
{
    LambdaSnail::resp::parser parser{};
};

TEST_P(ParserTypeTest, ParseTypes) {
    auto const params = GetParam();
    auto const result = parser.parse(std::get<char const*>(params));
    EXPECT_EQ(result.type, std::get<LambdaSnail::resp::data_type>(params));
}

INSTANTIATE_TEST_SUITE_P(
    ParserTypeTests,
    ParserTypeTest,
    testing::Values(
        std::tuple<char const*,LambdaSnail::resp::data_type>("+Hello World\r\n", LambdaSnail::resp::data_type::SimpleString),
        std::tuple<char const*,LambdaSnail::resp::data_type>(":12345678\r\n", LambdaSnail::resp::data_type::Integer),
        std::tuple<char const*,LambdaSnail::resp::data_type>(",123478\r\n", LambdaSnail::resp::data_type::Double),
        std::tuple<char const*,LambdaSnail::resp::data_type>(",123.478\r\n", LambdaSnail::resp::data_type::Double),
        std::tuple<char const*,LambdaSnail::resp::data_type>("#F\r\n", LambdaSnail::resp::data_type::Boolean),
        std::tuple<char const*,LambdaSnail::resp::data_type>("#t\r\n", LambdaSnail::resp::data_type::Boolean),
        std::tuple<char const*,LambdaSnail::resp::data_type>("_\r\n", LambdaSnail::resp::data_type::Null)
    ));

struct ParserErrorTest : public testing::TestWithParam<char const*>
{
    LambdaSnail::resp::parser parser{};
};

TEST_P(ParserErrorTest, ParseTypesError) {
    auto const params = GetParam();
    auto const result = parser.parse(params);
    EXPECT_EQ(result.type, LambdaSnail::resp::data_type::SimpleError);
}

INSTANTIATE_TEST_SUITE_P(
    ParserErrorTests,
    ParserErrorTest,
    testing::Values(
        "+Invalid ending\n",
        ":1234qwerty\r\n",
        ",1X3478\r\n",
        ",123.478",
        "#Ff\r\n",
        "#0\r\n",
        "_ABC\r\n"
    ));