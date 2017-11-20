alias(
    name = "bin",
    actual = ":foo",
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "foo",
    srcs = glob(["**/*.cpp"]),
    deps = [
        "//src/lib:lib"
    ],
    visibility = ["//visibility:public"],
)
