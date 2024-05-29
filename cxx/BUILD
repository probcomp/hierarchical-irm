licenses(["notice"])

cc_library(
    name = "headers",
    hdrs = glob(
        ["*.hh"],
        allow_empty = False,
    ),
    visibility = [":__subpackages__"],
)

cc_library(
    name = "cxxopts",
    srcs = ["cxxopts.hpp"],
    visibility = [":__subpackages__"],
    deps = [],
)

cc_binary(
    name = "hirm",
    srcs = ["hirm.cc"],
    deps = [
        ":cxxopts",
        ":headers",
        ":util_hash",
        ":util_io",
        ":util_math",
        "//distributions",
    ],
)

cc_library(
    name = "util_hash",
    hdrs = ["util_hash.hh"],
    deps = [],
)

cc_library(
    name = "util_io",
    srcs = ["util_io.cc"],
    visibility = [":__subpackages__"],
    hdrs = [
        "hirm.hh",
        "util_io.hh",
    ],
    deps = [
        ":headers",
        "//distributions",
    ],
)

cc_library(
    name = "util_math",
    srcs = ["util_math.cc"],
    hdrs = ["util_math.hh"],
    visibility = [":__subpackages__"],
    deps = [":headers"],
)

cc_test(
    name = "hirm_test",
    srcs = ["hirm_test.cc"],
    deps = [
        ":headers",
        "//distributions",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "util_math_test",
    srcs = ["util_math_test.cc"],
    deps = [
        ":util_math",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)
