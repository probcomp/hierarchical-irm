licenses(["notice"])

cc_library(
    name = "csv",
    hdrs = ["csv.hh"],
    srcs = ["csv.cc"],
    deps = [],
)

cc_test(
    name = "csv_test",
    srcs = ["csv_test.cc"],
    deps = [
        ":csv",
        "@boost//:test",
    ],
)

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
        ":schema",
        "//:cxxopts",
        "//:hirm_lib",
        "//:inference",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.hh"],
    srcs = ["schema.cc"],
    deps = [
        "//:irm",
    ],
)

cc_test(
    name = "schema_test",
    srcs = ["schema_test.cc"],
    deps = [
        ":io",
        ":schema",
        "@boost//:test",
    ],
)
