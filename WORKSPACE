workspace(name = "com_github_purecpp_raftcpp")

load("@com_github_purecpp_raftcpp//bazel:raftcpp_deps_setup.bzl", "raftcpp_deps_setup")

raftcpp_deps_setup()

load("@com_github_purecpp_raftcpp//bazel:raftcpp_deps_build_all.bzl", "raftcpp_deps_build_all")

raftcpp_deps_build_all()

# This needs to be run after grpc_deps() in metable_deps_build_all() to make
# sure all the packages loaded by grpc_deps() are available. However a
# load() statement cannot be in a function so we put it here.
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()
