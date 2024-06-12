licenses(["notice"])

cc_library(
    name = "distributions",
    visibility = ["//:__subpackages__"],
    deps = [
        ":adapter",
        ":base",
        ":beta_bernoulli",
        ":bigram",
        ":crp",
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
    srcs = ["beta_bernoulli.cc"],
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
    srcs = ["bigram.cc"],
    hdrs = ["bigram.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        ":dirichlet_categorical",
        "//:headers",
        "//:util_math",
    ],
)

cc_library(
    name = "crp",
    srcs = ["crp.cc"],
    hdrs = ["crp.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        "//:headers",
        "//:util_math",
    ],
)

cc_library(
    name = "dirichlet_categorical",
    srcs = ["dirichlet_categorical.cc"],
    hdrs = ["dirichlet_categorical.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//:util_math",
    ],
)

cc_library(
    name = "normal",
    srcs = ["normal.cc"],
    hdrs = ["normal.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//:util_math",
    ],
)
cc_library(
    name = "zero_mean_normal",
    srcs = ["zero_mean_normal.cc"],
    hdrs = ["zero_mean_normal.hh"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        "//:util_math",
    ],
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
        "@boost//:math",
        "@boost//:test",
    ],
)

cc_test(
    name = "bigram_test",
    srcs = ["bigram_test.cc"],
    deps = [
        ":bigram",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "crp_test",
    srcs = ["crp_test.cc"],
    deps = [
        ":crp",
        "@boost//:algorithm",
        "@boost//:test",
    ],
)

cc_test(
    name = "dirichlet_categorical_test",
    srcs = ["dirichlet_categorical_test.cc"],
    deps = [
        ":beta_bernoulli",
        ":dirichlet_categorical",
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
        "@boost//:math",
        "@boost//:test",
    ],
)

cc_test(
    name = "zero_mean_normal_test",
    srcs = ["zero_mean_normal_test.cc"],
    deps = [
        ":zero_mean_normal",
        "@boost//:algorithm",
        "@boost//:math",
        "@boost//:test",
    ],
)
