licenses(["notice"])

cc_library(
    name = "cxxopts",
    srcs = ["cxxopts.hpp"],
    visibility = [":__subpackages__"],
    deps = [],
)

cc_library(
    name = "clean_relation",
    hdrs = ["clean_relation.hh"],
    visibility = [":__subpackages__"],
    deps = [
        ":domain",
        ":relation",
        ":util_hash",
        ":util_math",
        "//distributions:get_distribution",
        "//emissions:get_emission"
    ],
)

cc_library(
    name = "domain",
    hdrs = ["domain.hh"],
    deps = [
        "//distributions:crp",
    ],
)

cc_library(
    name = "irm",
    hdrs = ["irm.hh"],
    srcs = ["irm.cc"],
    visibility = [":__subpackages__"],
    deps = [
        ":clean_relation",
        ":noisy_relation",
        ":transition_latent_value",
        "//distributions:get_distribution",
        "//emissions:get_emission"
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
        ":util_hash",
        ":util_io",
        ":util_math",
        "//distributions:get_distribution",
    ],
)

cc_binary(
    name = "typename_playground",
    srcs = ["typename_playground.cc"],
    deps = [
        ":clean_relation",
        ":domain",
        "//distributions:get_distribution",
    ],
)

cc_library(
    name = "noisy_relation",
    hdrs = ["noisy_relation.hh"],
    visibility = [":__subpackages__"],
    deps = [
        ":domain",
        ":clean_relation",
        ":relation",
        ":util_hash",
        ":util_math",
        "//distributions:get_distribution",
        "//emissions:get_emission",
    ],
)

cc_library(
    name = "relation",
    hdrs = ["relation.hh"],
    visibility = [":__subpackages__"],
    deps = [
        ":domain",
        ":util_hash",
        ":util_math",
        "//distributions:get_distribution",
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
        ":util_parse",
    ],
)

cc_library(
    name = "util_math",
    srcs = ["util_math.cc"],
    hdrs = ["util_math.hh"],
    visibility = [":__subpackages__"],
    deps = [],
)

cc_library(
    name = "util_parse",
    hdrs = ["util_parse.hh"],
    srcs = ["util_parse.cc"],
    visibility = [":__subpackages__"],
    deps = [],
)

cc_library(
    name = "util_observation",
    visibility = [":__subpackages__"],
    hdrs = ["util_observation.hh"],
    deps = [],
)

cc_library(
    name = "transition_latent_value",
    hdrs = ["transition_latent_value.hh"],
    visibility = [":__subpackages__"],
    deps = [
        "//emissions:base",
        ":relation",
        ":noisy_relation",
    ],
)

cc_test(
    name = "clean_relation_test",
    srcs = ["clean_relation_test.cc"],
    deps = [
        ":domain",
        ":clean_relation",
        "//distributions:beta_bernoulli",
        "//distributions:bigram",
        "@boost//:test",
    ],
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
    name = "hirm_test",
    srcs = ["hirm_test.cc"],
    deps = [
        ":hirm_lib",
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
    name = "noisy_relation_test",
    srcs = ["noisy_relation_test.cc"],
    deps = [
        ":domain",
        ":clean_relation",
        ":noisy_relation",
        "//distributions:beta_bernoulli",
        "//distributions:bigram",
        "@boost//:test",
    ],
)

filegroup(
    name = "testdata",
    srcs = [
        "test_schema/nary.schema",
        "test_schema/all_types.schema",
        "test_schema/with_noisy.schema",
    ],
)

cc_test(
    name = "util_io_test",
    srcs = ["util_io_test.cc"],
    data = [
        ":testdata",
    ],
    deps = [
        ":util_io",
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

cc_test(
    name = "util_parse_test",
    srcs = ["util_parse_test.cc"],
    deps = [
        ":util_parse",
        "@boost//:test",
    ],
)

cc_test(
    name = "transition_latent_value_test",
    srcs = ["transition_latent_value_test.cc"],
    deps = [
        ":transition_latent_value",
        ":clean_relation",
        ":domain",
        "//distributions:get_distribution",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)
