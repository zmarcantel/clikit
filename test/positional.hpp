#ifndef __POSITIONAL_TEST_HPP__
#define __POSITIONAL_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Positional, Single) {
    const char* argv[] = {"hello", "-n", "123", "foo"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t counts = 0;
    const char* file = nullptr;

    cli::Parser args(argc, argv);
    args.arg('n', "test", counts)
        .positional("file", "test", file);

    EXPECT_EQ(123, counts);
    EXPECT_EQ("foo", file);
}

TEST(Positional, Multiple) {
    const char* argv[] = {"hello", "foo", "-n", "123", "bar", "baz"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t counts = 0;
    const char* first = nullptr;
    const char* second = nullptr;
    const char* third = nullptr;

    cli::Parser args(argc, argv);
    args.arg('n', "test", counts)
        .positional("first", "test", first)
        .positional("second", "test", second)
        .positional("third", "test", third);

    EXPECT_EQ(123, counts);
    EXPECT_EQ("foo", first) << "wrong positional: " << first;
    EXPECT_EQ("bar", second) << "wrong positional: " << second;
    EXPECT_EQ("baz", third) << "wrong positional: " << third;
}

TEST(Positional, GatherAll) {
    const char* argv[] = {"hello", "foo", "-n", "123", "bar", "baz"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::size_t counts = 0;
    std::vector<const char*> files;

    cli::Parser args(argc, argv);
    args.arg('n', "test", counts)
        .all_positionals("files", "test", files);

    EXPECT_EQ(123, counts);
    ASSERT_EQ(3, files.size());
    EXPECT_EQ("foo", files[0]);
    EXPECT_EQ("bar", files[1]);
    EXPECT_EQ("baz", files[2]);
}


//-------------------------------------------------------------------------
// error testing
//-------------------------------------------------------------------------

TEST(Positional, ValidateUnused) {
    const char* argv[] = {"hello", "foo", "-n", "123", "bar", "baz"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::vector<const char*> files;

    cli::Parser args(argc, argv);
    EXPECT_THROW(
        args.all_positionals("files", "test", files),
        cli::ParseError
    );
}



#endif
