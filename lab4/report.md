# Operating Systems · Lab 4: The xv6 Shell from the Outside
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track B: Build your own small OS (xv6)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to systematically compare a production general-purpose shell (GNU Bash on Linux) with a minimal educational shell (`user/sh.c` on xv6). Specifically, the goals are:
1. Updating and verifying `sleep.c` using the current xv6 `pause()` system call.
2. Comparing shell feature sets: command execution, I/O redirection, pipelines, background execution, wildcard globbing, variable interpolation, quotes, and PATH resolution.
3. Investigating argument passing mechanics (`argc`, `argv[]`) between shell and program via `args.c` (xv6) and `args_linux.c` (Linux) to prove who performs wildcard expansion.
4. Analyzing the AST parser and command execution tree in `user/sh.c` (`forkexec`, `runcmd`, `parsecmd`).

---

## Part 1. Fix `sleep` for Current xv6

- Verified `user/user.h` system call definitions.
- Updated `user/sleep.c` to directly call `pause()` as required by the current xv6 kernel API.
- Ensured `$U/_sleep\` is registered in `UPROGS` in `Makefile`.

```
$ sleep 20
(pauses for 20 ticks and returns cleanly)

$ sleep
usage: sleep <ticks>
```
**Figure 4.1 – Execution of updated sleep utility inside xv6.**

---

## Part 2. bash Versus the xv6 Shell

### Comparison Table

| Command | Tests | bash (Linux) Output | xv6 Shell Output | Explanation |
|---|---|---|---|---|
| `echo hello > f ; cat < f` | Redirection | `hello` | `hello` | Both shells implement file descriptor redirection (`>` and `<`). |
| `ls \| grep READ` | Pipe | `README` | `README` | Both shells implement concurrent process execution with IPC pipes (`\|`). |
| `(echo a; echo b) \| wc` | Grouping, list | `2 2 4` | Syntax error / unexpected `(` | bash supports subshell command grouping `(...)`; xv6 shell does not recognize parentheses. |
| `echo one ; echo two` | Sequence | `one` <br> `two` | `one` <br> `two` | Both shells parse `;` as a command sequence separator (type `LIST`). |
| `echo bg &` | Background | `[1] 12345` <br> `bg` | `bg` | Both support background jobs via `&`; bash prints job/PID status, xv6 does not. |
| `ls *` | Globbing | Lists all files (`README`, ...) | `ls: cannot open *` | bash expands wildcards; xv6 has no globbing logic, passing the literal character `*`. |
| `echo $HOME` | Variables | `/home/student` | `$HOME` | bash performs variable substitution; xv6 does not support environment variables. |
| `echo "a b"` | Quotes | `a b` | `"a b"` | bash strips quoting characters; xv6 passes quotation marks literally as part of argv. |
| `mkdir d ; cd d ; ls ..` | PATH / Relative paths | Lists parent directory | `exec ls failed` | bash searches `$PATH`; xv6 only looks for `ls` in current directory unless given an absolute/relative path. |
| Arrow up, Tab, Ctrl+R | History, completion | Recalls history, autocompletes | Prints raw escape sequences `^[[A` | bash uses GNU Readline; xv6 shell has a primitive terminal driver with no history/line editing. |

### Observe Questions & Answers

1. **List the features both shells have, and the ones only bash has.**  
   - **Both shells support:** Command execution via `fork`/`exec`, I/O redirection (`<`, `>`), pipelines (`|`), command sequencing (`;`), and background execution (`&`).
   - **Only bash supports:** Wildcard expansion/globbing (`*`, `?`), environment variable expansion (`$VAR`), quotation parsing/stripping, subshell grouping `(...)`, `$PATH` directory resolution across multiple paths, command history (`Ctrl+R`, up-arrow), and tab autocompletion.

2. **In xv6, `ls` stops working after `cd d`. Why? Find a way to run `ls` from inside `d` anyway.**  
   xv6 does not maintain a `$PATH` environment variable. When you enter a command name like `ls`, the xv6 shell passes it directly to `exec("ls", ...)`, which only looks in the current working directory (`.`). Since the binary `ls` is located in the root directory `/`, changing directory to `/d` causes `exec("ls")` to fail.  
   - **Solution:** Execute it with an explicit path: `../ls` or `/ls`.

3. **Which missing feature would you miss most in daily work, and which would be easiest to add?**  
   - **Most missed:** Tab-completion and command history (arrow keys / Readline), because interactive terminal navigation is significantly slowed down without them.
   - **Easiest to add:** Basic quote stripping or `$PATH` searching (checking `/` if execution in `.` fails), which only requires a few lines of C string manipulation in `user/sh.c`.

---

## Part 3. Who Expands `*`? Program Printing `argv`

### Implementations:
- `user/args.c` for xv6
- `args_linux.c` for Linux

```
# Linux:
$ ./args_linux *
argc = 4
argv[0] = [./args_linux]
argv[1] = [args.c]
argv[2] = [args_linux.c]
argv[3] = [Makefile]

# xv6:
$ args *
argc = 2
argv[0] = [args]
argv[1] = [*]
```
**Figure 4.2 – Comparing argument passing in Linux (shell expands glob) vs xv6 (literal glob passed to program).**

### Observe Questions & Answers

1. **Did `args` in xv6 see `*` or the file names? Who did the expansion on Linux?**  
   In xv6, `args` saw the literal string `*`. On Linux, `args_linux` received the expanded list of file names (`args.c`, `args_linux.c`, etc.). The **shell (Bash)** performed the expansion on Linux before `execve` was called.

2. **When you type `rm *` in xv6, what happens? Compare with Linux.**  
   In xv6, `rm` receives `argv[1] = "*"`. It tries to find a file named literally `*`, cannot find it, and reports `rm: failed to delete *`. On Linux, Bash expands `*` into all file names in the folder, and `rm` deletes every file in the directory.

---

## Part 4. Look Under the Hood (`user/sh.c`)

### Observe Questions & Answers

1. **Find `runcmd` in `user/sh.c`. Which system call is used for `|`? Which for `<` and `>`?**  
   - For `|` (pipelines): `pipe()` is used to allocate the pipe, and `dup()` (via `close()` and `dup()`) remaps descriptors.
   - For `<` and `>` (redirection): `open()` is called with appropriate flags (`O_RDONLY` or `O_WRONLY|O_CREATE|O_TRUNC`), followed by `close()` and `dup()` to replace file descriptor 0 or 1.

2. **Why does `forkexec` (or `fork` in `sh.c`) run before `runcmd` for most commands, but `cd` is executed directly in the parent?**  
   Commands run in a child process so that program exit or crashes do not kill the shell, and file descriptor redirections do not affect the shell itself. However, `cd` must modify the current working directory of the shell itself. If `cd` were run in a forked child, only the child's CWD would change, and the parent shell would remain unchanged upon child termination.

3. **Search `sh.c` for any code that handles `*` or `$`. What does that explain about Part 2?**  
   Neither `*` nor `$` appears anywhere in `user/sh.c`. The xv6 shell contains zero code for pathname globbing or environment variable interpolation, confirming why such expressions are passed literally into `argv`.

---

## Conclusion (Concluzii proprii)

In this laboratory work, comparing the xv6 shell with GNU Bash provided clear insights into operating system design:
1. **Separation of Concerns:** Proved empirically via `args.c` and `args_linux.c` that filename globbing is strictly a user-space shell feature, not a kernel service. The kernel's `execve` simply receives an array of string pointers (`argv[]`).
2. **Minimalist Architecture:** The xv6 shell demonstrates how a complete, functioning UNIX shell supporting redirection, pipelines, and background execution can be constructed in under 500 lines of C using only `fork`, `exec`, `wait`, `pipe`, `dup`, and `close`.
3. **Shell Primitives:** Gained direct understanding of why `cd` must be a shell built-in and how pipeline synchronization relies fundamentally on EOF transmission over closed pipe file descriptors.
