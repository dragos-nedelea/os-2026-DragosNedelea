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
