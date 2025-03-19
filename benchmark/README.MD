# benchmark

接下来演示如何运行性能测试，注意shell代码里用`init_dir`指示该shell初始的path，`tinycoro_path`表示
tinycoro项目根目录的路径

## 测试工具准备

安装cargo

```shell
sudo apt-get install cargo
```

编译测试工具

```shell
# init_dir=tinycoro_path/build
make build-benchtools 
```

## 运行测试

由于压测工具与待测试程序的构建分离，需要运行下面的指令构建待测试程序

```shell
# init_dir=tinycoro_path/build
make build-bench
```

tinycoro提供了5种服务端模型供压测

- `bench_tinycoro_128`
- `bench_tinycoro_1k`
- `bench_tinycoro_16k`
- `bench_epoll_server`
- `bench_uring_server`

这里以负载1k，并发量为10为样例，用户开启两个终端分别为服务端和测试端

**step1.** 运行服务端程序，注意这里不用直接运行二进制文件，只需要`make`后跟上述的服务端模型的名字就行

```shell
# init_dir=tinycoro_path/build
make bench_tinycoro_1k
```


**step2.** 运行测试端程序

```shell
# init_dir=tinycoro_path
cd benchmark
bash bench.sh 10 1024
```

注意`bench.sh`需要接收两个参数，并发量和负载大小，负载大小默认以byte为单位。
