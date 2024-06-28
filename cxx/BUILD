licenses(["notice"])

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

cc_library(
    name = "irm",
    hdrs = ["irm.hh"],
    srcs = ["irm.cc"],
    visibility = [":__subpackages__"],
    deps = [
        ":relation",
        ":relation_variant",
        ":util_distribution_variant",
    ],
)

cc_library(
    name = "hirm_lib",
    hdrs = ["hirm.hh"],
    srcs = ["hirm.cc"],
    visibility = [":__subpackages__"],
    deps = [
        ":irm",
    ],
)

cc_binary(
    name = "hirm",
    srcs = ["hirm_main.cc"],
    deps = [
        ":cxxopts",
        ":hirm_lib",
        ":util_distribution_variant",
        ":util_hash",
        ":util_io",
        ":util_math",
        "//distributions",
    ],
)

cc_binary(
    name = "typename_playground",
    srcs = ["typename_playground.cc"],
    deps = [
        ":relation_variant",
        ":util_distribution_variant",
    ],
)

cc_library(
    name = "relation",
    hdrs = ["relation.hh"],
    visibility = [":__subpackages__"],
    deps = [
        ":domain",
        ":util_distribution_variant",
        ":util_hash",
        ":util_math",
        "//distributions:base"
    ],
)

cc_library(
    name = "relation_variant",
    hdrs = ["relation_variant.hh"],
    srcs = ["relation_variant.cc"],
    visibility = [":__subpackages__"],
    deps = [
        ":domain",
        ":relation",
        ":util_distribution_variant",
    ],
)

cc_library(
    name = "util_distribution_variant",
    srcs = ["util_distribution_variant.cc"],
    visibility = [":__subpackages__"],
    hdrs = ["util_distribution_variant.hh"],
    deps = [
        ":domain",
        "//distributions",
        "@boost//:algorithm",
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
        "util_io.hh",
    ],
    deps = [
        ":hirm_lib",
        "//distributions",
    ],
)

cc_library(
    name = "util_math",
    srcs = ["util_math.cc"],
    hdrs = ["util_math.hh"],
    visibility = [":__subpackages__"],
    deps = [],
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
    name = "irm_test",
    srcs = ["irm_test.cc"],
    deps = [
        ":irm",
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
    name = "relation_variant_test",
    srcs = ["relation_variant_test.cc"],
    deps = [
        ":relation_variant",
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
