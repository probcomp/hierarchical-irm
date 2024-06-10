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
    name = "gaussian",
    srcs = ["gaussian.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//distributions:zero_mean_normal",
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
