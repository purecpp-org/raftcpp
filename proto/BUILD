package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc//cpp:defs.bzl", "cpp_proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto_grpc//python:defs.bzl", "python_grpc_compile")

proto_library(
    name = "raft_proto",
    srcs = ["raft.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_protobuf//:empty_proto",
    ],
)

cc_proto_library(
    name = "raft_cc_proto",
    deps = [":raft_proto"],
)
