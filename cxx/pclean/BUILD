licenses(["notice"])

cc_library(
    name = "io",
    hdrs = ["io.hh"],
    srcs = ["io.cc"],
    deps = [
        ":parse",
        ":schema",
    ],
)

cc_test(
    name = "io_test",
    srcs = ["io_test.cc"],
    deps = [
        ":io",
        "@boost//:test",
    ],
)

cc_library(
    name = "parse",
    hdrs = ["parse.hh"],
    srcs = ["parse.cc"],
    deps = [],
)

cc_test(
    name = "parse_test",
    srcs = ["parse_test.cc"],
    deps = [
        ":parse",
        "@boost//:test",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.hh"],
    deps = [],
)
