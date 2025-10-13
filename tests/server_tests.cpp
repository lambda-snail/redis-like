#include "fuzztest/fuzztest.h"
#include "gtest/gtest.h"

import resp;
import server;

template<typename TData>
void StoresDataCorrectly(std::string const& key, TData const& val, std::function<void(std::string const&, TData const&)> expectation)
{
    LambdaSnail::server::server server(1);
    LambdaSnail::server::command_dispatch dispatch(server);

    std::vector<LambdaSnail::resp::v2::data> request{ "SET", key, val };
    std::ignore = dispatch.process_command(request);

    request[0] = "GET";
    request.pop_back();
    auto const& result = dispatch.process_command(request);

    expectation(result, val);
}

void StoresStringsCorrectly(std::string const& key, std::string const& val)
{
    StoresDataCorrectly<std::string>(key, val, [](std::string const& result, std::string const& value)
    {
        EXPECT_EQ(result, std::format("${}\r\n{}\r\n", value.size(), value));
    });
}

void StoresIntsCorrectly(std::string const& key, int64_t const& val)
{
    StoresDataCorrectly<int64_t>(key, val, [](std::string const& result, auto const& value)
    {
        EXPECT_EQ(result, std::format(":{}\r\n", value));
    });
}

// void StoresDoublesCorrectly(std::string const& key, double const& val)
// {
//     StoresDataCorrectly<double>(key, val, [](std::string const& result, auto const& value)
//     {
//         EXPECT_EQ(result, std::format(",{}\r\n", value));
//     });
// }

void StoresBooleansCorrectly(std::string const& key, bool const& val)
{
    StoresDataCorrectly<bool>(key, val, [](std::string const& result, auto const& value)
    {
        EXPECT_EQ(result, std::format("#{}\r\n", value?"t":"f"));
    });
}

FUZZ_TEST(DatabaseSetStringTests, StoresStringsCorrectly)
  .WithDomains(fuzztest::Arbitrary<std::string>(), fuzztest::Arbitrary<std::string>());

FUZZ_TEST(DatabaseSetStringTests, StoresIntsCorrectly)
  .WithDomains(fuzztest::Arbitrary<std::string>(), fuzztest::Arbitrary<int64_t>());

// FUZZ_TEST(DatabaseSetStringTests, StoresDoublesCorrectly)
//   .WithDomains(fuzztest::Arbitrary<std::string>(), fuzztest::Arbitrary<double>());

FUZZ_TEST(DatabaseSetStringTests, StoresBooleansCorrectly)
  .WithDomains(fuzztest::Arbitrary<std::string>(), fuzztest::Arbitrary<bool>());