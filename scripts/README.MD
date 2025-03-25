# 性能分析profile教程

tinycoro预置了使用perf一键生成火焰图的脚本`perf_svg.py`，本节教大家如何使用。

### 安装perf

对于linux系统，一般直接运行下列指令即可安装perf

```shell
sudo apt install linux-tools-generic
```

检查perf安装是否成功

```shell
perf -v
```

对于wsl，运行上述安装指令后运行perf系统会出现找不到命名的问题，此时用户用下列指令查找perf路径

```shell
find /usr/lib -name perf
```

将查找到的perf二进制文件拷贝到`/usr/bin`下面就可以正常使用perf了

### 火焰图生成脚本使用

在tinycoro项目的`scripts`文件夹下的`perf_svg.py`即生成脚本，比如用户对`./build/bin/hello`这个二进制文件进行性能分析，可以执行下列指令

```shell
python ./scripts/perf_svg.py ./build/bin/hello
```

运行成功的话svg文件默认会存储在tinycoro项目的`temp`目录下

使用浏览器打开svg文件可查看火焰图，效果如下：

![样例火焰图](../resource/exp_perf_svg.png)
