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
        ":schema_helper",
        "//:cxxopts",
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
        "//:hirm_lib",
        "//:util_io",
    ],
)

cc_test(
    name = "pclean_lib_test",
    srcs = ["pclean_lib_test.cc"],
    deps = [
        ":pclean_lib",
        "@boost//:test",
    ],
)

cc_library(
    name = "schema",
    hdrs = ["schema.hh"],
    deps = [],
)

cc_library(
    name = "schema_helper",
    hdrs = ["schema_helper.hh"],
    srcs = ["schema_helper.cc"],
    deps = [
        ":get_joint_relations",
        ":schema",
        "//:irm",
    ],
)


cc_test(
    name = "schema_helper_test",
    srcs = ["schema_helper_test.cc"],
    deps = [
        ":io",
        ":schema_helper",
        "@boost//:test",
    ],
)
