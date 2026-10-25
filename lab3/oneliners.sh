#!/bin/bash
# oneliners.sh - Lab 3 Solutions
# Runs with: bash oneliners.sh inside ~/os-lab3

# Q1. How many requests failed?
grep -c "FAIL" access.log

# Q2. How many different pages were requested, and which?
cut -d' ' -f5 access.log | sort -u

# Q3. How many requests did each page get?
cut -d' ' -f5 access.log | sort | uniq -c

# Q4. Which user or users have the most failed requests, and how many?
grep "FAIL" access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn | head -2

# Q5. How many requests did user3 make, and how many of them failed?
echo "user3 total: $(grep -c 'user3' access.log), failures: $(grep 'user3' access.log | grep -c 'FAIL')"

# Q6. Print the last 3 failed requests, showing only time and user.
grep "FAIL" access.log | tail -3 | awk '{print $2, $3}'

# Q7. Which login shells appear in /etc/passwd, and how many accounts use each? (field 7, separator :)
grep -v '^#' /etc/passwd | cut -d: -f7 | sort | uniq -c | sort -rn

# Q8. PREDICT, then explain: why do ls /etc | wc -l and ls -l /etc | wc -l differ by exactly one?
# Explanation: 'ls -l' prints a summary header line ("total <blocks>") at the beginning of its output, whereas bare 'ls' does not.
echo "ls /etc count: $(ls /etc | wc -l | tr -d ' '), ls -l /etc count: $(ls -l /etc | wc -l | tr -d ' ') (difference of 1 is the 'total' header line)"

# Bonus (+1): with grep -E , print only the lines where the minute ends in 0 (10:00, 10:10, ...) and the request failed. How many are there?
grep -E "10:[0-5]0 .* FAIL" access.log | wc -l | tr -d ' '
