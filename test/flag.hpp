#ifndef __FLAG_TEST_HPP__
#define __FLAG_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Flag, ShortAndLong) {
    const char* argv[] = {"hello", "-n", "--long"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    bool got_short = false;
    bool got_long = false;

    cli::Parser parse(argc, argv);
    parse.flag('n', "test", got_short)
         .flag("long", "test", got_long);

    EXPECT_EQ(got_short, true);
    EXPECT_EQ(got_long, true);
}

TEST(Flag, MatchEither) {
    const char* argv_short[] = {"hello", "-n"};
    std::size_t argc_short = sizeof(argv_short) / sizeof(argv_short[0]);

    bool got_short = false;
    cli::Parser parse_short(argc_short, argv_short);
    parse_short.flag('n', "number", "test", got_short);
    EXPECT_EQ(got_short, true);

    const char* argv_long[] = {"hello", "--number"};
    std::size_t argc_long = sizeof(argv_long) / sizeof(argv_long[0]);

    bool got_long = false;
    cli::Parser parse_long(argc_long, argv_long);
    parse_long.flag('n', "number", "test", got_long);
    EXPECT_EQ(got_long, true);
}

TEST(Flag, Invert) {
    const char* argv[] = {"hello", "-n",};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    bool inv = true;
    cli::Parser parse(argc, argv);
    parse.flag('n', "test", inv, true);

    EXPECT_EQ(inv, false);
}


//-------------------------------------------------------------------------
// error tests
//-------------------------------------------------------------------------

TEST(Flag, OnlySetOnce) {
    const char* argv[] = {"hello", "-n", "-n"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    bool got_short = false;
    cli::Parser parse(argc, argv);
    EXPECT_THROW(
        parse.flag('n', "test", got_short),
        cli::ParseError
    );

    EXPECT_EQ(got_short, true);
}

TEST(Flag, OnlySetOnceRun) {
    const char* argv[] = {"hello", "-nn"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    bool got_short = false;
    cli::Parser parse(argc, argv);
    EXPECT_THROW(
        parse.flag('n', "test", got_short),
        cli::ParseError
    );
}


#endif
