# one-way-north-shell
 os class design



https://mbinary.xyz/simple-shell.html

https://zhuanlan.zhihu.com/p/360923356

https://drustz.com/posts/2015/09/27/step-by-step-shell1/



全局变量State保存当前shell的各种上下文信息


specified:
命名规则：

变量：全小写，下划线

函数：驼峰，首字母小写

全局变量：首字母大写，下划线

结构体、宏定义：全大写，下划线

parse input to link table, to handle with |