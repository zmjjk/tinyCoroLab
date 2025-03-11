# benchmark

## 测试工具准备

安装cargo

```shell
sudo apt-get install cargo
```

编译项目

```shell
cd third_party/rust_echo_bench
cargo run --release -- --help
```

## 运行测试

这里以负载1k，并发量为10为样例，用户开启两个终端分别为服务端和测试端

运行服务端程序

```shell
cd build
./bin/tinycoro_bench_1k
```

运行测试端程序

```shell
cd third_party/rust_echo_bench/
../../benchmark/bench.sh 10 1k
```

注意bench.sh需要接收两个参数，并发量和负载大小，负载大小默认byte为单位，但请注意**如果负载达到kb或者mb级别请添加k或者m作为单位**。
