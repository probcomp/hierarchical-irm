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

cc_library(
    name = "domain",
    hdrs = ["domain.hh"],
    deps = [
        "//distributions",
    ],
)

cc_binary(
    name = "hirm",
    srcs = ["hirm.cc"],
    deps = [
        ":cxxopts",
        ":headers",
        ":util_distribution_variant",
        ":util_hash",
        ":util_io",
        ":util_math",
        "//distributions",
    ],
)

cc_library(
    name = "relation",
    hdrs = ["relation.hh"],
    deps = [
        ":domain",
        ":util_distribution_variant",
        ":util_hash",
        ":util_math",
        "//distributions:base"
    ],
)

cc_library(
    name = "util_distribution_variant",
    srcs = ["util_distribution_variant.cc"],
    visibility = [":__subpackages__"],
    hdrs = ["util_distribution_variant.hh"],
    deps = [
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
    name = "domain_test",
    srcs = ["domain_test.cc"],
    deps = [
        ":domain",
        "@boost//:test",
    ],
)

cc_test(
    name = "relation_test",
    srcs = ["relation_test.cc"],
    deps = [
        ":domain",
        ":relation",
        "//distributions",
        "@boost//:test",
    ],
)

cc_test(
    name = "util_distribution_variant_test",
    srcs = ["util_distribution_variant_test.cc"],
    deps = [
        ":util_distribution_variant",
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
