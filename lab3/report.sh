#!/bin/bash
# usage: ./report.sh access.log
log=$1

if [ ! -f "$log" ]; then
    echo "no such file: $log" >&2
    exit 1
fi

total=$(wc -l < "$log" | tr -d ' ')
fails=$(grep -c FAIL "$log")
echo "$total requests, $fails failed"

# TODO: for each of user0 ... user6, print "userN: X failures"
for u in $(seq 0 6); do
    user="user$u"
    count=$(grep -w "$user" "$log" | grep -c "FAIL")
    echo "$user: $count failures"
done
