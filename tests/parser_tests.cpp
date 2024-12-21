import resp;

#include <gtest/gtest.h>

namespace ParseValidTests
{
    template<typename T>
    struct ValidRespStringTestFixture : public testing::Test {};//WithParam<char const*>
    // {
    //     using Type = RespType;
    // };

    struct TestInt
    {
        char const* data = ":1234\r\n";
        int64_t expected = 1234;
        LambdaSnail::resp::Integer type;
    };

    struct TestNegativeInt
    {
        char const* data = ":-1234\r\n";
        int64_t expected = -1234;
        LambdaSnail::resp::Integer type;
    };

    struct TestDouble
    {
        char const* data = ",1.234\r\n";
        double expected = 1.234;
        LambdaSnail::resp::Double type;
    };

    struct TestNegativeDouble
    {
        char const* data = ",-1.234\r\n";
        double expected = -1.234;
        LambdaSnail::resp::Double type;
    };

    TYPED_TEST_SUITE_P(ValidRespStringTestFixture);

    TYPED_TEST_P(ValidRespStringTestFixture, TestMaterializeValidResp)
    {
        TypeParam test_data;
        LambdaSnail::resp::data_view view(test_data.data);
        auto value = view.materialize(test_data.type);
        ASSERT_TRUE(value == test_data.expected);

        // typename TypeParam::first_type param1;
        // typename TypeParam::second_type param2;
        // ASSERT_TRUE(param1.ch == std::toupper(param2.ch));
    }

    REGISTER_TYPED_TEST_SUITE_P(ValidRespStringTestFixture, TestMaterializeValidResp);

    using ValidRespStringTest_Types = ::testing::Types<TestInt, TestNegativeInt, TestDouble, TestNegativeDouble>;
    INSTANTIATE_TYPED_TEST_SUITE_P(XXYYZZ,ValidRespStringTestFixture,ValidRespStringTest_Types);
}



// TYPED_TEST_SUITE_P(
//     ValidRespStringTest,
//     ValidRespStringTestFixture,
//     testing::Values(
//         "+Hello World\r\n",
//         ":12345678\r\n",
//         ":-12345678\r\n",
//         ",123478\r\n",
//         ",-123478\r\n",
//         ",123.478\r\n",
//         "#F\r\n",
//         "#t\r\n",
//         "_\r\n"
//     ));

// TYPED_TEST_SUITE_P(ValidRespStringTest);
//
// using ValidRespStringTest_Types = ::testing::Types<char, int, unsigned int>;
// TYPED_TEST_P(ValidRespStringTest, ValidRespStringTest_Types);

// TYPED_TEST_P(ValidRespStringTest, XXXXX) {
//
//
//     //auto const result = parser.parse_message_s(std::get<char const*>(params));
//     //EXPECT_EQ(result.type, std::get<LambdaSnail::resp::data_type>(params));
// };

// struct ParserErrorTest : public testing::TestWithParam<char const*>
// {
//     LambdaSnail::resp::parser parser{};
// };
//
// TEST_P(ParserErrorTest, ParseTypesError) {
//     auto const params = GetParam();
//     auto const result = parser.parse_message_s(params);
//     EXPECT_EQ(result.type, LambdaSnail::resp::data_type::SimpleError);
// }
//
// INSTANTIATE_TEST_SUITE_P(
//     ParserErrorTests,
//     ParserErrorTest,
//     testing::Values(
//         "+Invalid ending\n",
//         ":1234qwerty\r\n",
//         ":1234-5\r\n",
//         ",1X3478\r\n",
//         ",123.478",
//         ",-12-3.478\r\n",
//         "#Ff\r\n",
//         "#0\r\n",
//         "_ABC\r\n"
//     ));