load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def raftcpp_deps_setup():
    maybe(
        http_archive,
        name = "com_github_spdlog",
        sha256 = "1e68e9b40cf63bb022a4b18cdc1c9d88eb5d97e4fd64fa981950a9cacf57a4bf",
        urls = [
            "https://github.com/gabime/spdlog/archive/v1.8.0.tar.gz",
        ],
        strip_prefix = "spdlog-1.8.0",
        build_file = "@com_github_purecpp_raftcpp//bazel/thirdparty:spdlog.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        url = "https://github.com/grpc/grpc/archive/refs/tags/v1.47.0.tar.gz",
        strip_prefix = "grpc-1.47.0",
    )

    maybe(
        git_repository,
        name = "rules_proto_grpc",
        branch = "dev",
        remote = "https://github.com/rules-proto-grpc/rules_proto_grpc.git",
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        url = "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
        sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
        strip_prefix = "googletest-release-1.11.0",
    )

    maybe(
        http_archive,
        name = "com_github_gflags_gflags",
        url = "https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz",
        sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        strip_prefix = "gflags-2.2.2",
    )

    maybe(
        http_archive,
        name = "asio",
        urls = [
            "https://udomain.dl.sourceforge.net/project/asio/asio/1.22.1%20%28Stable%29/asio-1.22.1.tar.gz",
        ],
        strip_prefix = "asio-1.22.1",
        build_file = "@com_github_purecpp_raftcpp//bazel/thirdparty:asio.BUILD",
    )
