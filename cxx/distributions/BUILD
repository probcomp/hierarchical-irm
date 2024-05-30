licenses(["notice"])


cc_library(
    name = "distributions",
    visibility = ["//:__subpackages__"],
    deps = [
        ":adapter",
        ":base",
        ":bigram",
        ":beta_bernoulli",
        ":dirichlet_categorical",
        ":normal",
    ],
)

cc_library(
    name = "adapter",
    hdrs = ["adapter.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
    ],
)

cc_library(
    name = "base",
    hdrs = ["base.hh"],
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "beta_bernoulli",
    hdrs = ["beta_bernoulli.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//:headers",
        "//:util_math",
    ],
)

cc_library(
    name = "bigram",
    hdrs = ["bigram.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//:headers",
        "//:util_math",
    ],
)

cc_library(
    name = "dirichlet_categorical",
    srcs = ["dirichlet_categorical.hh"],
    visibility = ["//:__subpackages__"],
    deps = [":base"],
)

cc_library(
    name = "normal",
    srcs = ["normal.hh"],
    visibility = ["//:__subpackages__"],
    deps = [":base"],
)

cc_test(
    name = "adapter_test",
    srcs = ["adapter_test.cc"],
    deps = [
        ":adapter",
        ":normal",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "beta_bernoulli_test",
    srcs = ["beta_bernoulli_test.cc"],
    deps = [
        ":beta_bernoulli",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "normal_test",
    srcs = ["normal_test.cc"],
    deps = [
        ":normal",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)
