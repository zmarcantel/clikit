alias(
    name = "all",
    actual = ":clikit_test",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "clikit_test",
    srcs = glob(["**/*.cpp", "**/*.hpp"]),
    deps = [
        "@googletest//:gtest_main",
        "//src:lib",
    ],
    visibility = ["//visibility:public"],
)
