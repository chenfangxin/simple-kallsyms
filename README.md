simple-kallsyms
===============

基于Linux内核中的kallsyms技术，用于输出用户空间程序的调用栈或异常栈信息。

使用示例：
make
./demo

kill -12 demo_pid

实现原理
--------------
1. 原程序中声明weak属性的变量
2. 使用外部工具，提取程序中的函数名及其地址信息
3. 利用连接器，将提取的信息链接进原程序
4. 在信号处理函数中，输出调用栈

