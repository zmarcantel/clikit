alias(
    name = "lib",
    actual = ":foo",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "foo",
    srcs = glob(["**/*.cpp"]),
    hdrs = [
        "lib.hpp",
    ],
    deps = [
    ],
    visibility = ["//visibility:public"],
)
