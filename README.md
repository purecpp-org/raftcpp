# raftcpp
A RAFT implementation to help build your RAFT service in 1 minute.  
[Note that this project is now WORKING IN PROGRESS. We are going to release 0.1.0 soon.]

## Main dependencies
- bazel: for building tool
- asio: for asynchronous eventloop and IO
- grpc: for RPC framework
- protobuf: for serialization framework
- gtest: for test framework
- spdlog: for logging tool
- gflags: for command line tool

## Quick Start
### Install Building Tool
We are now using bazel 5.1.1 for building `raftcpp` project, please make sure you have installed bazel. If you don't have installed it, you could install it via the following command:
```shell script
./scripts/install-bazel.sh
```

### Build
```shell script
bazel build //:all
```
### Test
```shell script
bazel test //:all
```
or test one specific test case in following command:
```shell script
bazel test //:xxxx_test
```

## Get Involved
Because this project is working in progress now, we are very welcome you if 
you have the willing to contribute any thing.

1. Open issue to feedback issues or create pull request for your code changes.
2. After your code changed, maybe a unit test is necessary.
3. Run `./scripts/code-format.sh` to format your code style.
4. Review, then merge.
