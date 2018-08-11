#ifndef __SUBCOMMAND_TEST_HPP__
#define __SUBCOMMAND_TEST_HPP__

#include "gtest/gtest.h"
#include "src/clikit.hpp"

TEST(Subcommand, NoIntermediateWithTrailingGlobal) {
    const char* argv[] = {"hello", "build", "-n", "123", "-n=456", "-n=098"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::string subcommand;
    std::vector<std::size_t> counts;

    cli::Parser parse(argc, argv);
    parse.list('n', "test", counts)
        .subcommand("build", "test subcommand", subcommand);

    ASSERT_EQ(counts.size(), 3);
    EXPECT_EQ(counts[0], 123);
    EXPECT_EQ(counts[1], 456);
    EXPECT_EQ(counts[2], 98);

    ASSERT_EQ(subcommand, "build");
}

TEST(Subcommand, NoIntermediateWithPrecedingGlobal) {
    const char* argv[] = {"hello", "-n", "123", "-n=456", "-n=098", "build"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::string subcommand;
    std::vector<std::size_t> counts;

    cli::Parser parse(argc, argv);
    parse.list('n', "test", counts)
        .subcommand("build", "test subcommand", subcommand);

    ASSERT_EQ(counts.size(), 3);
    EXPECT_EQ(counts[0], 123);
    EXPECT_EQ(counts[1], 456);
    EXPECT_EQ(counts[2], 98);

    ASSERT_EQ(subcommand, "build");
}

TEST(Subcommand, Arguments) {
    const char* argv[] = {"hello", "test", "-v", "-ffff"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::string subcommand;

    bool build_verbose = false;
    bool test_verbose = false;

    std::vector<std::string> build_files;
    std::size_t test_fidelity = 0;

    cli::Parser parse(argc, argv);
    parse
        .subcommand("build", "test subcommand", subcommand)
            .flag('v', "verbose", "build verbosity", build_verbose)
            .list('f', "files", "build files", build_files)
            .done()
        .subcommand("test", "test subcommand", subcommand)
            .flag('v', "verbose", "test verbosity", test_verbose)
            .count('f', "fidelity", "test fidelity", test_fidelity)
            .done()
    ;

    EXPECT_EQ(subcommand, "test");

    EXPECT_FALSE(build_verbose);
    EXPECT_TRUE(test_verbose);

    EXPECT_EQ(0, build_files.size());
    EXPECT_EQ(4, test_fidelity);
}

TEST(Subcommand, NestedSubcommands) {
    const char* argv[] = {"hello", "build", "-v", "-f", "foo.c", "-f=bar.c", "release", "-l", "4"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::string subcommand;
    std::string build_subcommand;

    bool build_verbose = false;
    bool test_verbose = false;

    std::vector<std::string> build_files;
    std::size_t test_fidelity = 0;

    std::size_t opt_level = 0;
    bool strip = false;

    cli::Parser parse(argc, argv);
    parse
        .subcommand("build", "build subcommand", subcommand)
            .flag('v', "verbose", "build verbosity", build_verbose)
            .list('f', "files", "build files", build_files)
            .subcommand("release", "", build_subcommand)
                .arg('l', "level", "optimization level", opt_level)
                .done()
            .subcommand("debug", "", build_subcommand)
                .flag('s', "strip", "strip symbols", strip)
                .done()
            .done()
        .subcommand("test", "test subcommand", subcommand)
            .flag('v', "verbose", "build verbosity", test_verbose)
            .count('f', "fidelity", "test fidelity", test_fidelity)
            .done()
    ;

    EXPECT_EQ(subcommand, "build");
    EXPECT_EQ(build_subcommand, "release");

    EXPECT_TRUE(build_verbose);
    EXPECT_FALSE(test_verbose);

    EXPECT_EQ(2, build_files.size());
    EXPECT_EQ("foo.c", build_files[0]);
    EXPECT_EQ("bar.c", build_files[1]);
    EXPECT_EQ(0, test_fidelity);

    EXPECT_EQ(4, opt_level);
    EXPECT_FALSE(strip);
}


TEST(Subcommand, ToBoolean) {
    const char* argv[] = {"hello", "build", "release"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    bool is_build = false;
    bool is_release = false;
    bool is_debug = false;

    bool is_test = false;

    cli::Parser parse(argc, argv);
    parse
        .subcommand("build", "build subcommand", is_build)
            .subcommand("release", "", is_release)
                .done()
            .subcommand("debug", "", is_debug)
                .done()
            .done()
        .subcommand("test", "test subcommand", is_test)
            .done()
    ;

    EXPECT_TRUE(is_build);
    EXPECT_TRUE(is_release);
    EXPECT_FALSE(is_debug);
    EXPECT_FALSE(is_test);
}

TEST(Subcommand, ToList) {
    const char* argv[] = {"hello", "build", "release", "clean"};
    std::size_t argc = sizeof(argv) / sizeof(argv[0]);

    std::vector<std::string> subcommands;

    cli::Parser parse(argc, argv);
    parse
        .subcommand("build", "build subcommand", subcommands)
            .subcommand("release", "", subcommands)
                .subcommand("clean", "", subcommands)
                .done()
            .subcommand("debug", "", subcommands)
                .done()
            .done()
        .subcommand("test", "test subcommand", subcommands)
            .done()
    ;

    ASSERT_EQ(3, subcommands.size());
    EXPECT_EQ("build", subcommands[0]);
    EXPECT_EQ("release", subcommands[1]);
    EXPECT_EQ("clean", subcommands[2]);
}

//-------------------------------------------------------------------------
// error testing
//-------------------------------------------------------------------------

#endif
