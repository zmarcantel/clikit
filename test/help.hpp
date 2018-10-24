#ifndef __HELP_TEST_HPP__
#define __HELP_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Help, TriggeredShort) {
    const char* argv[] = {"hello", "-h"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    cli::Parser parse(argc, argv);
    parse.details("hello", "just a hello world tool we can use");

    EXPECT_TRUE(parse.wants_help());
}

TEST(Help, TriggeredLong) {
    const char* argv[] = {"hello", "--help"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::uint8_t verbosity = 0;
    std::vector<std::string> files;

    cli::Parser parse(argc, argv);
    parse.details("hello", "just a hello world tool we can use")
        .version("v0.1")
        .count('v', "add a verbosity level", verbosity)
        .list('f', "file", "files to load", files, "FILE")
    ;

    EXPECT_TRUE(parse.wants_help());
}


//-------------------------------------------------------------------------
// error testing
//-------------------------------------------------------------------------



#endif
