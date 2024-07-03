licenses(["notice"])

cc_library(
    name = "emissions",
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        ":get_emission",
        ":bitflip",
        ":gaussian",
        ":simple_string",
        ":sometimes",
    ],
)

cc_library(
    name = "base",
    hdrs = ["base.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        "//distributions:base",
    ],
)

cc_library(
    name = "get_emission",
    srcs = ["get_emission.cc"],
    hdrs = ["get_emission.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":bitflip",
        ":gaussian",
        ":simple_string",
        ":sometimes",
        "//:util_observation",
    ],
)

cc_library(
    name = "bitflip",
    srcs = ["bitflip.hh"],
    visibility = ["//:__subpackages__"],
    deps = [":base"],
)

cc_library(
    name = "categorical",
    srcs = ["categorical.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//distributions:dirichlet_categorical",
    ],
)

cc_library(
    name = "gaussian",
    srcs = ["gaussian.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//distributions:zero_mean_normal",
    ],
)

cc_library(
    name = "simple_string",
    srcs = ["simple_string.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//distributions:beta_bernoulli",
    ],
)

cc_library(
    name = "sometimes",
    srcs = ["sometimes.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//distributions:beta_bernoulli",
    ],
)

cc_test(
    name = "get_emission_test",
    srcs = ["get_emission_test.cc"],
    deps = [
        ":get_emission",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "bitflip_test",
    srcs = ["bitflip_test.cc"],
    deps = [
        ":bitflip",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "categorical_test",
    srcs = ["categorical_test.cc"],
    deps = [
        ":categorical",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)
cc_test(
    name = "gaussian_test",
    srcs = ["gaussian_test.cc"],
    deps = [
        ":gaussian",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "simple_string_test",
    srcs = ["simple_string_test.cc"],
    deps = [
        ":bitflip",
        ":simple_string",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "sometimes_test",
    srcs = ["sometimes_test.cc"],
    deps = [
        ":bitflip",
        ":sometimes",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)
