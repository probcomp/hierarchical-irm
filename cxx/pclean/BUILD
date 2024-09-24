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
    name = "get_joint_relations",
    hdrs = ["get_joint_relations.hh"],
    srcs = ["get_joint_relations.cc"],
    deps = [
        ":schema",
        "//:clean_relation",
        "//:noisy_relation",
    ],
)

cc_test(
    name = "get_joint_relations_test",
    srcs = ["get_joint_relations_test.cc"],
    deps = [
        ":get_joint_relations",
        "@boost//:test",
    ],
)

cc_library(
    name = "io",
    hdrs = ["io.hh"],
    srcs = ["io.cc"],
    visibility = ["//:__subpackages__"],
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
        ":csv",
        ":io",
        ":pclean_lib",
        ":schema",
        "//:cxxopts",
        "//:gendb",
        "//:hirm_lib",
        "//:inference",
        "//:util_io",
    ],
)

cc_library(
    name = "pclean_lib",
    hdrs = ["pclean_lib.hh"],
    srcs = ["pclean_lib.cc"],
    deps = [
        ":csv",
        ":schema",
        "//:gendb",
        "//:hirm_lib",
        "//:util_io",
    ],
)

cc_test(
    name = "pclean_lib_test",
    srcs = ["pclean_lib_test.cc"],
    deps = [
        ":io",
        ":pclean_lib",
        "@boost//:test",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.hh"],
    visibility = ["//:__subpackages__"],
    deps = [],
)
