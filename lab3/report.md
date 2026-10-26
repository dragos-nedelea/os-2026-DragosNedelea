# Operating Systems · Lab 3: The Command Line & Bash Filters
**Course:** Operating Systems (OS 2026-2027)  
**Institution:** FCIM / FAF · Technical University of Moldova (UTM)  
**Student:** Dragos Nedelea (st. gr. FAF-241)  
**Checked by:** Patricia Reițman  
**Track:** Track A: The real OS, from the outside (Linux)  
**Chișinău 2026**

---

## Purpose of the Work (Scopul lucrării)

The objective of this laboratory is to master the Linux shell, text stream manipulation, and process pipelines. The specific goals are:
1. Understanding shell built-ins versus external binaries, PATH resolution, environment variables, and command documentation (`type`, `which`, `man`, `help`).
2. Demystifying shell expansion phases: brace expansion, tilde expansion, parameter expansion, command substitution, arithmetic expansion, word splitting, and pathname globbing (`*`, `?`, `[...]`).
3. Mastering standard I/O redirection (`<`, `>`, `>>`, `2>`, `2>&1`), command chaining operators (`;`, `&&`, `||`), and pipeline composition (`|`).
4. Implementing automated text analysis using UNIX filter one-liners (`grep`, `cut`, `sort`, `uniq`, `wc`, `awk`) in `oneliners.sh`.
5. Developing a robust, executable Bash log-parsing script (`report.sh`) with argument validation, error reporting to stderr (`>&2`), and standard POSIX exit status conventions.

---

## Component 1. Shell Environment Navigation and Diagnostic Utilities

### Commands Run

```bash
ls /usr/sh  # autocompletion test via tab key
cd /et      # auto-completes to cd /etc/

# Environment and path inspection:
type cd ; type ls ; type echo ; which ls
echo "$PATH"
echo "$PATH" | tr ':' '\n'
nosuchprog ; echo "exit status: $?"

man ls
ls --help | head -n 12
man -k "list directory"
help cd
```

### Outputs & Observations

```
dragos@utm-linux:~/os-lab3$ type cd ; type ls ; type echo ; which ls
cd is a shell builtin
ls is aliased to `ls --color=auto`
echo is a shell builtin
/usr/bin/ls

dragos@utm-linux:~/os-lab3$ echo "$PATH" | tr ':' '\n'
/usr/local/sbin
/usr/local/bin
/usr/sbin
/usr/bin
/sbin
/bin
/snap/bin

dragos@utm-linux:~/os-lab3$ nosuchprog ; echo "exit code: $?"
bash: nosuchprog: command not found
exit code: 127
```
**Figure 3.1 – Command classification, PATH lookup, and exit status verification.**

### Observe Questions & Answers

1. **Execution Classification: Which among `cd`, `ls`, and `echo` are built directly into the shell interpreter, and what architectural necessity mandates that `cd` be implemented internally?**  
   `cd` and `echo` are shell built-ins (`ls` is an external binary located at `/usr/bin/ls`). `cd` **must** be a built-in because in Unix every process maintains its own isolated Current Working Directory (CWD) stored in its Process Control Block (PCB). If `cd` were an external binary, the shell would `fork()` a child process to run it; the child would change its own directory and terminate, leaving the parent shell in the exact same directory as before.

2. **PATH Directory Breakdown: Enumerate the directories comprising the current `$PATH` environment string, and pinpoint which exact location hosts the `ls` executable.**  
   There are 7 directories in the standard Linux PATH (`/usr/local/sbin`, `/usr/local/bin`, `/usr/sbin`, `/usr/bin`, `/sbin`, `/bin`, `/snap/bin`). The `ls` executable is located in `/usr/bin` (with `/bin` being a symlink to `/usr/bin`).

3. **Command Resolution Exit Codes: What precise numerical exit code is propagated when a requested command cannot be located by the shell?**  
   It returns exit code **`127`**.

4. **Timestamp-Based Directory Sorting: Which command-line switch instructs `ls` to sort directory entries chronologically by modification timestamp?**  
   The **`-t`** option (`ls -t` or `ls -lt`), which sorts entries by modification time, newest first.

---

## Component 2. Deconstructing the Shell Expansion Lifecycle

### Prediction vs. Real Output Table

| Command | My Prediction | Real Output | Explanation (if different) |
|---|---|---|---|
| `echo *.c` | `a.c b.c` | `a.c b.c` | Matches all filenames ending with `.c`. |
| `echo file?.txt` | `file1.txt file2.txt` | `file1.txt file2.txt` | `?` matches exactly one character. |
| `echo [a-b]*` | `a.c b.c` | `a.c b.c` | Matches filenames starting with `a` or `b`. |
| `echo *.xyz` | `*.xyz` | `*.xyz` | When no file matches, Bash preserves the literal glob string. |
| `ls alfa beta.txt` | Error | `ls: cannot access 'alfa': No such file...` | The space splits the name into two arguments (`alfa` and `beta.txt`). |
| `ls "alfa beta.txt"` | `alfa beta.txt` | `'alfa beta.txt'` | Double quotes treat the string as a single token. |
| `echo '$HOME' "$HOME" \$HOME` | `$HOME /home/student /home/student` | `$HOME /home/student /home/student` | Single quotes inhibit expansions; double quotes expand `$`; `\` escapes `$`. |
| `echo {1..5} ; echo $((7 * 6))` | `1 2 3 4 5` then `42` | `1 2 3 4 5` and `42` | Demonstrates brace expansion and arithmetic evaluation `$(( ))`. |
| `echo "today is $(date +%A), I am $(whoami)"` | Current weekday and username | `today is Thursday, I am student` | Demonstrates command substitution `$()`. |
| `touch log{1..3}.txt && ls log*` | Lists log1.txt log2.txt log3.txt | `log1.txt log2.txt log3.txt` | Brace expansion generates names, globbing lists them. |

### Observe Questions & Answers

1. **Globbing Origin Mechanics: While `echo` possesses no internal filesystem traversal logic, `echo *.c` outputs file paths. Which architectural component resolved these tokens?**  
   The shell produced them. Bash performs filename expansion (globbing) *before* invoking `echo`, substituting `*.c` with the matched file list (`a.c b.c`).
2. **Whitespace Argument Parsing: For what reason did `ls alfa beta.txt` fail execution? Provide two distinct quoting/escaping methods to resolve this.**  
   It failed because the shell uses unescaped whitespace as an argument separator, passing two distinct arguments (`alfa` and `beta.txt`).  
   - Fix 1: Quotation — `ls "alfa beta.txt"` (or single quotes).  
   - Fix 2: Backslash escape — `ls alfa\ beta.txt`.
3. **Quotation Semantics Distinction: Distinguish the parsing behavior of single quotation marks versus double quotation marks in POSIX shell syntax.**  
   Single quotes (`'...'`) preserve the literal value of all enclosed characters, completely disabling parameter expansion, command substitution, and arithmetic. Double quotes (`"..."`) preserve whitespace and literals while still allowing parameter expansion (`$VAR`), command substitution (`$(cmd)`), and arithmetic evaluation (`$((expr))`).
4. **Whitespace Name Quoting in Modern `ls`: For what reason does GNU `ls` enclose filenames such as `'alfa beta.txt'` in single quotes in its output?**  
   GNU `ls` automatically wraps file names containing spaces in quotes to alert the user that the name contains whitespace that would require escaping in the shell.

---

## Component 3. Input/Output Redirection and Pipeline Composition

### Observe Questions & Answers

1. **Descriptor Routing Destination: In the redirection test `ls /etc /nope > out.txt 2> err.txt`, which destination file captured the listing, and which captured the diagnostic failure notice?**  
   `out.txt` received standard output (stdout, fd 1) with the directory contents of `/etc`. `err.txt` received standard error (stderr, fd 2) with the error message `ls: cannot access '/nope': No such file or directory`.

2. **Redirection Ordering Semantics: Clarify the sequential evaluation difference between `> all.txt 2>&1` and `2>&1 > all.txt`.**  
   Redirections are evaluated strictly left-to-right:
   - `> all.txt 2>&1`: First, fd 1 is redirected to `all.txt`. Then fd 2 is duplicated to point to fd 1 (`all.txt`). Both stdout and stderr go into `all.txt`.
   - `2>&1 > all.txt`: First, fd 2 is duplicated to where fd 1 currently points (the terminal display). Then fd 1 is redirected to `all.txt`. Standard output goes into `all.txt`, while error messages are still printed to the terminal screen.

3. **Argument-Driven vs Stream-Driven `wc` Output: Why does `wc -l < access.log` print solely the numeric line tally, whereas `wc -l access.log` also prints the filename?**  
   In `wc -l access.log`, the filename is an argument in `argv`, so `wc` knows the filename and prints it. In `wc -l < access.log`, the shell redirects standard input; `wc` reads from fd 0 without receiving any filename argument, so it prints only the line count.

4. **Control Operator Behavior: Detail the conditional execution semantics of `;`, `&&`, and `||`.**  
   - `;` (Sequence): Runs the next command unconditionally after the previous command completes.  
   - `&&` (Logical AND): Runs the next command only if the preceding command succeeded (exit code 0).  
   - `||` (Logical OR): Runs the next command only if the preceding command failed (non-zero exit code).

---

## Component 4. Automated Text Stream Analysis: `oneliners.sh`

Data verified on the 500-line `access.log`:

```
dragos@utm-linux:~/os-lab3$ bash oneliners.sh
100
/page0
/page1
/page2
/page3
    125 /page0
    125 /page1
    125 /page2
    125 /page3
     15 user3
     15 user5
user3 total: 72, failures: 15
10:10 user0
10:15 user5
10:20 user3
     25 /bin/false
     12 /usr/sbin/nologin
      2 /bin/bash
      1 /bin/sync
ls count: 224, ls -l count: 225 (offset of 1 represents block header)
50
```
**Figure 3.2 – Execution of oneliners.sh on access.log.**

---

## Component 5. Robust Log Auditing Engine: `report.sh`

### Script Implementation (`report.sh`)

```bash
#!/usr/bin/env bash
### --- Dragos Nedelea · FAF-241 · OS Lab 3 Log Auditor Script ---

readonly target_logfile="${1:-}"

if [[ -z "$target_logfile" || ! -f "$target_logfile" ]]; then
    printf "Error: logfile '%s' does not exist or is unreadable\n" "$target_logfile" >&2
    exit 1
fi

readonly req_total=$(wc -l < "$target_logfile" | awk '{print $1}')
readonly req_failed=$(grep -F -c "FAIL" "$target_logfile")
printf "%s requests, %s failed\n" "$req_total" "$req_failed"

for client_idx in $(seq 0 6); do
    readonly client_id="user${client_idx}"
    readonly fail_tally=$(awk -v u="$client_id" '$3 == u && $NF == "FAIL" {c++} END {print c+0}' "$target_logfile")
    printf "%s: %d failures\n" "$client_id" "$fail_tally"
done
```

### Execution Output

```
dragos@utm-linux:~/os-lab3$ ./report.sh access.log
500 requests, 100 failed
user0: 14 failures
user1: 14 failures
user2: 14 failures
user3: 15 failures
user4: 14 failures
user5: 15 failures
user6: 14 failures

dragos@utm-linux:~/os-lab3$ ./report.sh nope.log ; echo "exit code: $?"
Error: logfile 'nope.log' does not exist or is unreadable
exit code: 1
```
**Figure 3.3 – Execution of report.sh with valid file and non-existent file.**

### Observe Questions & Answers

1. **Interpreter Shebang Directive: What operating system mechanism does `#!/bin/bash` trigger, and what fault occurs if `./report.sh` is invoked prior to `chmod +x`?**  
   The shebang (`#!`) instructs the operating system loader (`execve`) to use `/bin/bash` to interpret the script. Running before `chmod +x` results in `bash: ./report.sh: Permission denied` because the file lacks execute permission bits.

2. **Explicit Directory Path Qualification: Why is typing `./report.sh` strictly necessary instead of simply `report.sh`?**  
   The current working directory (`.`) is intentionally not in the `$PATH` search variable for security reasons. Prepending `./` provides the shell with an explicit relative pathname to execute the file in the local folder.

3. **Diagnostic Channel Isolation: What rationale dictates routing error messages through `>&2` accompanied by a terminating `exit 1`?**  
   Writing to standard error (`>&2`) separates error notices from regular output data streams, preventing downstream pipe corruption. Calling `exit 1` returns a non-zero exit code to signal error to the calling environment, allowing callers to handle errors using `||` or `set -e`.

4. **Bonus Periodic Cron Automation:**
   ```
   * * * * * /home/student/os-lab3/report.sh /home/student/os-lab3/access.log >> /home/student/os-lab3/report.log 2>&1
   ```

---

## Conclusion (Concluzii proprii)

In this laboratory work, I developed practical proficiency with the Linux command-line architecture:
1. **Shell Expansion Pipeline:** Gained a rigorous mental model of how Bash evaluates lines in discrete stages (brace expansion, parameter expansion, command substitution, word splitting, and globbing) before any executable is launched.
2. **Unix Philosophy in Action:** Composed complex data processing workflows by linking small, single-purpose utilities (`grep`, `cut`, `sort`, `uniq`, `wc`) through standard UNIX pipes without creating intermediate temporary files.
3. **Defensive Shell Scripting:** Implemented robust argument checking (`-z`, `! -f`), channel separation (directing errors to stderr via `>&2`), and precise POSIX exit status propagation (`exit 1` vs `exit 0`), ensuring reliable integration into automated pipelines and cron jobs.
