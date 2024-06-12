licenses(["notice"])

cc_library(
    name = "base",
    hdrs = ["base.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        "//distributions:base",
    ],
)

cc_library(
    name = "bitflip",
    srcs = ["bitflip.hh"],
    visibility = ["//:__subpackages__"],
    deps = [":base"],
)

cc_library(
    name = "gaussian",
    srcs = ["gaussian.hh"],
    visibility = ["//:__subpackages__"],
    deps = [":base"],
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
    name = "bitflip_test",
    srcs = ["bitflip_test.cc"],
    deps = [
        ":bitflip",
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

# TODO(thomaswc): Fix and re-enable.
#cc_test(
#    name = "gaussian_test",
#    srcs = ["gaussian_test.cc"],
#    deps = [
#        ":gaussian",
#        "@boost//:algorithm",
#        "@boost//:test",
#    ],
#)
