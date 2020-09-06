# raftcpp
An implementation of Raft consensus algorithm in modern C++.  
[Note that this project is now working in progress.]

## Main dependencies
- asio
- rest_rpc
- doctest
- gflags

## Quick Start
### Build
```shell script
./build.sh
```
### Test
```shell script
cd build
./xxx_text
```
or run all tests with the following command:
```shell script
./run_all_tests.sh
```

## Get Involved
1. Open issue to feedback issues or create pull request for your code changes.
2. After your code changed, maybe a unit test is necessary.
3. Run `./scripts/code-format.sh` to format your code style.
4. Review, then merge.