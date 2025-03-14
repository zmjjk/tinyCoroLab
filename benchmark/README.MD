# benchmark

## 测试工具准备

安装cargo

```shell
sudo apt-get install cargo
```

编译测试工具

```shell
cd third_party/rust_echo_bench
cargo run --release -- --help
```

## 运行测试

由于压测与主程序的构建分离，需要运行下面的指令构建压测程序

```shell
make build-bench 
```

tinycoro提供了5种服务端模型供压测

- **bench_tinycoro_128**
- **bench_tinycoro_1k**
- **bench_tinycoro_16k**
- **bench_epoll_server**
- **bench_uring_server**

这里以负载1k，并发量为10为样例，用户开启两个终端分别为服务端和测试端

运行服务端程序，注意这里不用直接运行二进制文件，只需要make后跟上述的服务端模型的名换就行

```shell
make bench_tinycoro_1k
```


运行测试端程序

```shell
cd third_party/rust_echo_bench/
../../benchmark/bench.sh 10 1k
```

注意bench.sh需要接收两个参数，并发量和负载大小，负载大小默认以byte为单位。
