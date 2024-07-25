licenses(["notice"])

cc_library(
    name = "io",
    hdrs = ["io.hh"],
    srcs = ["io.cc"],
    deps = [
        "//:util_parse",
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

cc_binary(
    name = "pclean",
    srcs = ["pclean.cc"],
    deps = [
        ":io",
        ":translate_schema",
        "//:cxxopts",
        "//:hirm_lib",
        "//:inference",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.hh"],
    deps = [],
)

cc_library(
    name = "translate_schema",
    hdrs = ["translate_schema.hh"],
    srcs = ["translate_schema.cc"],
    deps = [
        ":schema",
        "//:irm",
    ],
)
