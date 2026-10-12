# Operating Systems · Lab 4: The xv6 Shell from the Outside
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea  
**Track:** Track B: Build your own small OS (xv6)

---

## Part 1. Fix `sleep` for Current xv6

- Verified `user/user.h` system call definitions.
- Updated `user/sleep.c` to use `pause()` if running on post-August 2025 xv6 trees, or `sleep()` on standard trees.
- Ensured `$U/_sleep\` is registered in `UPROGS` in `Makefile`.
- Verification inside xv6:
  ```
  $ sleep 20
  (pauses and returns)
  $ sleep
  usage: sleep <ticks>
  ```

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
- `args_linux.c` for Linux (compiled with `gcc -o args args_linux.c`)

### Observe Questions & Answers

1. **Paste both outputs for `args *.c "x y"`:**

   - **Linux bash (`./args *.c "x y"`):**
     ```
     argc = 4
     argv[0] = [./args]
     argv[1] = [args_linux.c]
     argv[2] = [sleep.c]
     argv[3] = [x y]
     ```
   - **xv6 (`args *.c "x y"`):**
     ```
     argc = 3
     argv[0] = [args]
     argv[1] = [*.c]
     argv[2] = ["x y"]
     ```

2. **So who turns `*.c` into file names and removes the quotes: the program, the kernel, or the shell?**  
   The **shell** performs these transformations! Before invoking the executable, the shell parses the line, expands wildcards against directory entries, interprets and strips quotation marks, and passes the resulting clean array of strings to the kernel via the `execve` system call. The program and kernel merely receive the final expanded `argv` array.

3. **What happened to the extra spaces in `args one    two    three`?**  
   The shell treats continuous whitespace sequences as argument delimiters, tokenizing the input into separate strings (`"one"`, `"two"`, `"three"`) and discarding the extra spaces.

---

## Part 4. Read the Shell (`user/sh.c`)

### Observe Questions & Answers

1. **Name the five kinds of command the xv6 shell knows and the character that produces each:**
   From `#define` directives in `user/sh.c`:
   - `EXEC` (1): Simple executable command (words separated by whitespace).
   - `REDIR` (2): Redirection command, produced by `<` (input) or `>` (output).
   - `PIPE` (3): Pipeline command, produced by `|`.
   - `LIST` (4): Sequential command list, produced by `;`.
   - `BACK` (5): Background asynchronous command, produced by `&`.

2. **Where is `cd` handled, and why is it not run as a separate program?**  
   `cd` is handled directly in `main()` of `user/sh.c`:
   ```c
   if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
     buf[strlen(buf)-1] = 0;  // chop \n
     if(chdir(buf+3) < 0)
       fprintf(2, "cannot cd %s\n", buf+3);
     continue;
   }
   ```
   It cannot be a separate program because `chdir` modifies only the current working directory of the process that invokes it. If executed as a child process via `fork()`, the child would change its directory and terminate, leaving the parent shell's directory unchanged.

3. **Search `sh.c` for any code that handles `*` or `$`. What does that explain about Part 2?**  
   Neither `*` nor `$` appears anywhere in `user/sh.c`. The xv6 shell does not contain any code for pathname pattern matching (globbing) or environment variable interpolation. Consequently, strings like `*.c` or `$HOME` are treated as literal text and passed unmodified into `argv`.
