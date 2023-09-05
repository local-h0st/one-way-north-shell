# one-way-north-shell
 os class design



https://mbinary.xyz/simple-shell.html

https://zhuanlan.zhihu.com/p/360923356

https://drustz.com/posts/2015/09/27/step-by-step-shell1/


// https://bmoos.github.io/2020/01/22/%E5%9C%A8Linux%E7%8E%AF%E5%A2%83%E4%B8%8B%E7%94%A8c%E5%AE%9E%E7%8E%B0%E7%AE%80%E6%98%93shell%E7%A8%8B%E5%BA%8F/
// https://bmoos.github.io/2020/01/15/fork/

https://www.cnblogs.com/mickole/p/3187409.html

全局变量State保存当前shell的各种上下文信息


specified:
命名规则：

变量：全小写，下划线

函数：驼峰，首字母小写

全局变量：首字母大写，下划线

结构体、宏定义 and enum：全大写，下划线

parse input to link table, to handle with |

child process forked, the output of the child is still put to stdout, meaning the screen. The output together doesn't mean execute together.