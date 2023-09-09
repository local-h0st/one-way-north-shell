# one-way-north-shell
Operating System class design powered by redh3tALWAYS

***preview***

![preview]()

### Naming Rules Specified
vars: example_var

global vars: Example_global_vars

functions: exampleFunc

struct & #define & enum: EXAMPLE_EXAMPLE

### Data Structure
```
struct STATE_STRUCT {
    char cwd[STATE_CWD_LEN];
    char input[STATE_INPUT_LEN];
    struct passwd *current_passwd;
    struct COMMAND_FRAG *command;
} State;

struct COMMAND_FRAG {
    char *binname;
    char *arg;
    struct COMMAND_FRAG *pipe_next;
    struct COMMAND_FRAG *arg_next;
    // TODO redirect
};

```

Global var `State` saves all the needed context information.

The raw user input `State.input` will be parsed into link table `State.command`.

To handle with pipe `|`, parse function will do these: if it meets a `|`, the next word will be treated as a binname and put into struct `COMMAND_FRAG`, and the the struct will be linked after the last `COMMAND_FRAG` struct's pipe_next properity.


### About Redirection
Grammas are hard to recognize, we assume your input simple.

As I test on kali's zsh, `echo 'test' 1 > a.txt && cat a.txt` finally gets `test 1`, and `echo 'test' 1 > a.txt && cat a.txt`gets `test`, that means there shouldn't be any space between fd and redirection symbol, but there can be space after the symbol.

`cat | wc -l < README.md` will stuck on kali zsh, meaning README.md won't be treated as input to `cat`. Redirection needs the highest priviledge.

**features:**

Now only support grammas like: `> file`, `>file`, `>> file`, `>>file`, `< file`, `<file`, `<< file`, `<file`.

At least one blank is needed before redirection symbol.

Currently doesn't support fd.


### About Quots
If you wanna make ` `, `|`, `<`, `>` chars be recognized as one params, use `"` to include them. `"` will be thrown in function executeOnce(), just before they are send to be executed. If you just wanna input `"`, use `\"`.

If you use `echo "aaa\"bbb"`, you will get the output like `aaa\"bbb`, this is nothing to do with my shell, my shell parse the `\"` correctly in the param's pass, this might because of how the `echo` handle with the single `"`.

### Things Not So Important

**Some Thinking:**

child process forked, the output of the child is still put to the screen ( stdout ). Child and parent output together doesn't mean they execute together.

the pipe given by pipe() func has direction, never change it. nowhere to search, I can fix it only because I thought out this.

in parent, wait can also be replaced with:
```
do{
    wpid = waitpid(pid, &status, WNOHANG|WUNTRACED);
} while (!WIFEXITED(status) && !WIFSIGNALED(status));
```

**Fixed bugs:**

The first time I use pipe() func to create pipe I refered to many blogs. I thought the examples given by other's blogs are werid, I think data should get into a pipe from p[0] and go out from p[1], but examples are the opposite. At first I followed my thought and error occurs, then I thought to myself, will the pipe has directions? Actually indeed, after my attempt to swap the read port and the write port. The pipe created by pipe() is not just two normal fds, they have directions, stdout/stderr should be redirected to p[1] and stdin should ben read from p[0].

Here's another.

I ever encountered a problem, that when I use pipe like `ls /proc | grep 2`, my shell will stuck after grep return its results. First I thought it might because what `ls` puts to pipe doesn't have a EOF flag, so `grep` thinks it haven't meet the end and it keeps on waiting for the EOF. So I test the execute function no longer in a loop but one step once.I already know pipe end need close, but I can't figure out when to close as the examples given by blogs are also hard to understand ( e.g they will close the write end after dup2, or in any other place they close the read end, I just don't know why the close the read/write end there, that is not a common place to close files ). When I was wondering when to close the pipe fd ( both read end and write end ), and I occasionally close the read end before execute `ls`, I found that even `ls` stuck. Then I realized the status of one end can influence the other end. Thus `grep` stuck on read pipe might because the pipe's write end is not closed, that means the unclosed write end will stuck read. It is later proved right with my futher tests. 

After fixed this, I found if a command contains more than 2 ( or 3? can't remember ) pipes, the output is still unexpected. I debug it by tracing pipes' fds in vscode and find when containing many pipes, the array `int **pipes` will have the same fd in two different pipes `pipes[0]` and `pipes[1]`, for example, there might be `pipes[0][1] == pipes[1][0] == 4`. Though I temporarily could't think of the consequence of this phenomenon, but it isn't a pleasant phenomenon at the first glance anyway. So I rearranged the order of my code to avid two pipes to have the same fd, and the bug went away, problem well solved.

Finally `ls /proc |grep 7| grep 8|grep 86 |grep 2 passed!` passed my code and executed successfully!

**References:**
[1](https://bmoos.github.io/2020/01/22/%E5%9C%A8Linux%E7%8E%AF%E5%A2%83%E4%B8%8B%E7%94%A8c%E5%AE%9E%E7%8E%B0%E7%AE%80%E6%98%93shell%E7%A8%8B%E5%BA%8F/), [2](https://drustz.com/posts/2015/09/27/step-by-step-shell1/), [3](https://mbinary.xyz/simple-shell.html), [4](https://developer.aliyun.com/article/990596), [5](https://www.cnblogs.com/mickole/p/3187409.html), [6](https://zhuanlan.zhihu.com/p/360923356)
