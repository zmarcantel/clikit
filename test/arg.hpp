#ifndef __ARG_TEST_HPP__
#define __ARG_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Arg, ShortSeparateArgs) {
    const char* argv[] = {"hello", "-n", "123"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.arg('n', "test", count);

    EXPECT_EQ(count, 123);
}

TEST(Arg, ShortWithEq) {
    const char* argv[] = {"hello", "-n=123"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.arg('n', "test", count);

    EXPECT_EQ(count, 123);
}

TEST(Arg, LongSeparateArgs) {
    const char* argv[] = {"hello", "--count", "123"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.arg("count", "test", count);

    EXPECT_EQ(count, 123);
}

TEST(Arg, LongWithEq) {
    const char* argv[] = {"hello", "--count=123"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.arg("count", "test", count);

    EXPECT_EQ(count, 123);
}


//-------------------------------------------------------------------------
// error testing
//-------------------------------------------------------------------------

TEST(Arg, OnlyProvideOnce) {
    const char* argv[] = {"hello", "-n", "123", "-n", "456"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    EXPECT_THROW(
        parse.arg('n', "test", count),
        cli::ParseError
    );
}

TEST(Arg, MustHaveArg) {
    const char* argv[] = {"hello", "-n"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count;

    cli::Parser parse(argc, argv);
    EXPECT_THROW(
        parse.arg('n', "test", count),
        cli::ParseError
    );
}



#endif
