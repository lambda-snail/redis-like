#include <variant>

#include <gtest/gtest.h>

import resp;
//
// // namespace ParseValidTests
// // {
// //     template<typename T>
// //     struct RespStringTestFixture : public testing::Test {};//WithParam<char const*>
// //     // {
// //     //     using Type = RespType;
// //     // };
// //
// //     struct TestInt
// //     {
// //         char const* data = ":1234\r\n";
// //         int64_t expected = 1234;
// //         LambdaSnail::resp::Integer type;
// //     };
// //
// //     struct TestNegativeInt
// //     {
// //         char const* data = ":-1234\r\n";
// //         int64_t expected = -1234;
// //         LambdaSnail::resp::Integer type;
// //     };
// //
// //     struct TestDouble
// //     {
// //         char const* data = ",1.234\r\n";
// //         double expected = 1.234;
// //         LambdaSnail::resp::Double type;
// //     };
// //
// //     struct TestNegativeDouble
// //     {
// //         char const* data = ",-1.234\r\n";
// //         double expected = -1.234;
// //         LambdaSnail::resp::Double type;
// //     };
// //
// //     struct TestBool
// //     {
// //         char const* data = "#T\r\n";
// //         bool expected = true;
// //         LambdaSnail::resp::Boolean type;
// //     };
// //
// //     struct TestSimpleString
// //     {
// //         char const* data = "+INCR\r\n";
// //         std::string expected = "INCR";
// //         LambdaSnail::resp::SimpleString type;
// //     };
// //
// //     struct TestBulkString
// //     {
// //         char const* data = "$4\r\nINCR\r\n";
// //         std::string expected = "INCR";
// //         LambdaSnail::resp::BulkString type;
// //     };
// //
// //     struct TestBulkStringWithLineEndings
// //     {
// //         char const* data = "$20\r\nINCR\r\nThe other line\r\n";
// //         std::string expected = "INCR\r\nThe other line";
// //         LambdaSnail::resp::BulkString type;
// //     };
// //
// //     TYPED_TEST_SUITE_P(RespStringTestFixture);
// //
// //     TYPED_TEST_P(RespStringTestFixture, TestMaterializeValidResp)
// //     {
// //         TypeParam test_data;
// //         LambdaSnail::resp::data_view view(test_data.data);
// //         auto value = view.materialize(test_data.type);
// //         ASSERT_TRUE(value == test_data.expected);
// //     }
// //
// //     REGISTER_TYPED_TEST_SUITE_P(RespStringTestFixture, TestMaterializeValidResp);
// //
// //     using ValidRespStringTest_Types = ::testing::Types<
// //         TestInt, TestNegativeInt,
// //         TestDouble, TestNegativeDouble,
// //         TestBool,
// //         TestSimpleString, TestBulkString, TestBulkStringWithLineEndings
// //     >;
// //
// //     INSTANTIATE_TYPED_TEST_SUITE_P(TestMaterializeValidResp,RespStringTestFixture,ValidRespStringTest_Types);
// // }
//
TEST(parserTests, TestEmptyArray) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto const read = p.add_buffer({}, data_);

    EXPECT_EQ(read, 0);
    EXPECT_EQ(data_.size(), 0);
}

TEST(parserTests, TestArrayWithInteger) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto const read = p.add_buffer(":1234\r\n", data_);

    EXPECT_EQ(read, 7);
    EXPECT_EQ(data_.size(), 1);
    EXPECT_TRUE(p.is_done());
    EXPECT_EQ(std::get<int64_t>(data_[0]), 1234);
}

TEST(parserTests, TestArrayWithInteger_Continuation) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto read = p.add_buffer(":1234", data_);
    EXPECT_EQ(read, 5);
    EXPECT_FALSE(p.is_done());

    read = p.add_buffer("567\r\n", data_);
    EXPECT_EQ(read, 5);

    EXPECT_EQ(data_.size(), 1);
    EXPECT_TRUE(p.is_done());
    EXPECT_EQ(std::get<int64_t>(data_[0]), 1234567);
}

TEST(parserTests, TestArrayWithInteger_TwoContinuations) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto read = p.add_buffer(":-1234", data_);
    EXPECT_EQ(read, 6);
    EXPECT_FALSE(p.is_done());

    read = p.add_buffer("567", data_);
    EXPECT_EQ(read, 3);
    EXPECT_FALSE(p.is_done());

    read = p.add_buffer("89\r\n", data_);
    EXPECT_EQ(read, 4);

    EXPECT_EQ(data_.size(), 1);
    EXPECT_TRUE(p.is_done());
    EXPECT_EQ(std::get<int64_t>(data_[0]), -123456789);
}

TEST(parserTests, TestSimpleString_EmptyString) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto const read = p.add_buffer("+\r\n", data_);

    EXPECT_EQ(read, 3);
    EXPECT_TRUE(p.is_done());
    EXPECT_EQ(data_.size(), 1);
    EXPECT_TRUE(std::get<std::string>(data_[0]).empty());
}

TEST(parserTests, TestSimpleString_OnePass) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto const read = p.add_buffer("+Hello World\r\n", data_);

    EXPECT_EQ(read, 14);
    EXPECT_TRUE(p.is_done());
    EXPECT_EQ(data_.size(), 1);
    EXPECT_EQ(std::get<std::string>(data_[0]), "Hello World");
}

TEST(parserTests, TestSimpleString_TerminationInLastPass) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto read = p.add_buffer("+Hello World", data_);
    EXPECT_EQ(read, 12);
    EXPECT_FALSE(p.is_done());

    read = p.add_buffer("\r\n", data_);
    EXPECT_EQ(read, 2);
    EXPECT_TRUE(p.is_done());

    EXPECT_EQ(data_.size(), 1);
    EXPECT_EQ(std::get<std::string>(data_[0]), "Hello World");
}

TEST(parserTests, TestSimpleString_TwoPasses) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto read = p.add_buffer("+Hello ", data_);
    EXPECT_EQ(read, 7);
    EXPECT_FALSE(p.is_done());

    read = p.add_buffer("World\r\n", data_);
    EXPECT_EQ(read, 7);
    EXPECT_TRUE(p.is_done());

    EXPECT_EQ(data_.size(), 1);
    EXPECT_EQ(std::get<std::string>(data_[0]), "Hello World");
}

TEST(parserTests, TestMixedValues_StringAndInt_OnePass) {
    LambdaSnail::resp::v2::parser p;

    std::vector<LambdaSnail::resp::v2::data> data_{};
    auto read = p.add_buffer("+Hello World\r\n:1234\r\n", data_);
    EXPECT_EQ(read, 21);
    EXPECT_TRUE(p.is_done());

    EXPECT_EQ(data_.size(), 2);
    EXPECT_EQ(std::get<std::string>(data_[0]), "Hello World");
    EXPECT_EQ(std::get<int64_t>(data_[1]), 1234);
}

//
//
// // TYPED_TEST_SUITE_P(
// //     ValidRespStringTest,
// //     ValidRespStringTestFixture,
// //     testing::Values(
// //         "+Hello World\r\n",
// //         ":12345678\r\n",
// //         ":-12345678\r\n",
// //         ",123478\r\n",
// //         ",-123478\r\n",
// //         ",123.478\r\n",
// //         "#F\r\n",
// //         "#t\r\n",
// //         "_\r\n"
// //     ));
//
// // TYPED_TEST_SUITE_P(ValidRespStringTest);
// //
// // using ValidRespStringTest_Types = ::testing::Types<char, int, unsigned int>;
// // TYPED_TEST_P(ValidRespStringTest, ValidRespStringTest_Types);
//
// // TYPED_TEST_P(ValidRespStringTest, XXXXX) {
// //
// //
// //     //auto const result = parser.parse_message_s(std::get<char const*>(params));
// //     //EXPECT_EQ(result.type, std::get<LambdaSnail::resp::data_type>(params));
// // };
//
// // struct ParserErrorTest : public testing::TestWithParam<char const*>
// // {
// //     LambdaSnail::resp::parser parser{};
// // };
// //
// // TEST_P(ParserErrorTest, ParseTypesError) {
// //     auto const params = GetParam();
// //     auto const result = parser.parse_message_s(params);
// //     EXPECT_EQ(result.type, LambdaSnail::resp::data_type::SimpleError);
// // }
// //
// // INSTANTIATE_TEST_SUITE_P(
// //     ParserErrorTests,
// //     ParserErrorTest,
// //     testing::Values(
// //         "+Invalid ending\n",
// //         ":1234qwerty\r\n",
// //         ":1234-5\r\n",
// //         ",1X3478\r\n",
// //         ",123.478",
// //         ",-12-3.478\r\n",
// //         "#Ff\r\n",
// //         "#0\r\n",
// //         "_ABC\r\n"
// //     ));