#ifndef __LIST_TEST_HPP__
#define __LIST_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(List, Shorts) {
    const char* argv[] = {"hello", "-n", "123", "-n=456", "-n=098"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::vector<std::size_t> counts;

    cli::Parser parse(argc, argv);
    parse.list('n', "test", counts);

    ASSERT_EQ(counts.size(), 3);
    EXPECT_EQ(counts[0], 123);
    EXPECT_EQ(counts[1], 456);
    EXPECT_EQ(counts[2], 98);
}

TEST(List, Longs) {
    const char* argv[] = {"hello", "--count", "123", "--count=456", "--count=098"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::vector<std::size_t> counts;

    cli::Parser parse(argc, argv);
    parse.list("count", "test", counts);

    ASSERT_EQ(counts.size(), 3);
    EXPECT_EQ(counts[0], 123);
    EXPECT_EQ(counts[1], 456);
    EXPECT_EQ(counts[2], 98);
}

//-------------------------------------------------------------------------
// error testing
//-------------------------------------------------------------------------


TEST(List, MustHaveArg) {
    const char* argv[] = {"hello", "-n"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::vector<std::size_t> counts;

    cli::Parser parse(argc, argv);
    EXPECT_THROW(
        parse.list('n', "test", counts),
        cli::ParseError
    );
}


#endif
