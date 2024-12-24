[![C++20](https://img.shields.io/badge/dialect-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)
[![MIT license](https://img.shields.io/github/license/max0x7ba/atomic_queue)](https://github.com/sakurs2/tinyCoro/blob/master/LICENSE)
<br>
![platform Linux x86_64](https://img.shields.io/badge/platform-Linux%20x86_64--bit-yellow)
# tinyCoro
tinyCoro是利用C++20提供的协程技术封装的协程库，意图提供较异步io更便捷，性能更优的库支持，开发者可基于该库仅用数十行代码搭建自己的web服务器。内部采用one context one thread的模式进行任务调度。