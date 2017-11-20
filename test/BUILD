alias(
    name = "all",
    actual = ":foo_test",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "foo_test",
    srcs = glob(["**/*.cpp"]),
    deps = [
        "@googletest//:gtest_main",
        "//src/lib:lib",
    ],
    visibility = ["//visibility:public"],
)
