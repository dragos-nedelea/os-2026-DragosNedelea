#!/usr/bin/env bash
### --- Dragos Nedelea · FAF-241 · OS Lab 3 Stream Processing Solutions ---

# Task 1: Total tally of failed HTTP queries in the log
awk '$NF == "FAIL" { count++ } END { print count }' access.log

# Task 2: Distinct target endpoints requested across all client transactions
awk '{ print $5 }' access.log | sort --unique

# Task 3: Distribution frequency of access per individual resource URL
awk '{ print $5 }' access.log | sort | uniq -c

# Task 4: Identity of users with highest recorded failure frequency
awk '$NF == "FAIL" { print $3 }' access.log | sort | uniq -c | sort -k1,1nr | head -n 2

# Task 5: Complete transaction volume and failure proportion for client user3
printf "user3 total: %d, failures: %d\n" \
  "$(awk '$3 == "user3"' access.log | wc -l)" \
  "$(awk '$3 == "user3" && $NF == "FAIL"' access.log | wc -l)"

# Task 6: Chronological tail entries (time and user) for the final 3 failures
awk '$NF == "FAIL" { printf "%s %s\n", $2, $3 }' access.log | tail -n 3

# Task 7: Shell binary distribution for active system accounts in /etc/passwd
awk -F: '!/^#/ && NF >= 7 { print $7 }' /etc/passwd | sort | uniq -c | sort -k1,1nr

# Task 8: Discrepancy analysis between unadorned and detailed directory counts
# Note: 'ls -l' prepends an informational "total <blocks>" metric before listing directory entries
printf "ls count: %d, ls -l count: %d (offset of 1 represents block header)\n" \
  "$(ls -1 /etc | wc -l)" \
  "$(ls -l /etc | wc -l)"

# Extra Credit Challenge (+1): Failed records logged on ten-minute boundaries (XX:X0)
grep -E '^[0-9]+ [0-9]{2}:[0-5]0 [^ ]+ [^ ]+ [^ ]+ FAIL' access.log | wc -l
