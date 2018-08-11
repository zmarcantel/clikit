#ifndef __COUNT_TEST_HPP__
#define __COUNT_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Count, ShortAndLong) {
    const char* argv[] = {"hello", "-n", "-n", "--count", "--count", "--count"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.count('n', "count", "test", count);

    EXPECT_EQ(count, 5);
}

TEST(Count, Run) {
    const char* argv[] = {"hello", "-vvvvv"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.count('v', "verb", "verbosity", count);

    EXPECT_EQ(count, 5);
}

TEST(Count, RunMixed) {
    const char* argv[] = {"hello", "-vxvpxvxpvxvpxv"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count_v = 0;
    std::size_t count_p = 0;
    std::size_t count_x = 0;

    cli::Parser parse(argc, argv);
    parse.count('v', "verb", "verbosity", count_v)
        .count('p', "party-time", "other test thing", count_p)
        .count('x', "extreme", "other test thing", count_x);

    EXPECT_EQ(count_v, 6);
    EXPECT_EQ(count_p, 3);
    EXPECT_EQ(count_x, 5);
}

TEST(Count, MultipleRuns) {
    const char* argv[] = {"hello", "-vvv", "-vvv"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t count = 0;

    cli::Parser parse(argc, argv);
    parse.count('v', "verb", "verbosity", count);

    EXPECT_EQ(count, 6);
}


//-------------------------------------------------------------------------
// error tests
//-------------------------------------------------------------------------

#endif
