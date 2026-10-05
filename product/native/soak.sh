#!/bin/sh
trap "" HUP
cd /mnt/SDCARD/Apps/CodexNative || exit 1
mkdir -p evidence
initial=$(pidof gm-native)
[ -n "$initial" ] || exit 1
start=$(date +%s)
deadline=$((start + 7200))
printf 'time,pid,rss_kb,cache_age,selected\n' > evidence/soak.csv
while [ "$(date +%s)" -lt "$deadline" ]; do
 now=$(date +%s)
 pid=$(pidof gm-native)
 if [ "$pid" != "$initial" ] || [ -z "$pid" ]; then printf 'FAIL_PID,%s,%s\n' "$now" "$pid" >> evidence/soak.events; fi
 rss=$(awk '/VmRSS:/ { print $2 }' "/proc/$pid/status" 2>/dev/null)
 selected=$(cat selected.id 2>/dev/null | tr -cd 'a-zA-Z0-9-')
 age=$(/tmp/gm-device-check cache-age)
 printf '%s,%s,%s,%s,%s\n' "$now" "$pid" "$rss" "$age" "$selected" >> evidence/soak.csv
 sleep 20
done
printf 'COMPLETE start=%s end=%s duration=%s initial_pid=%s final_pid=%s\n' "$start" "$(date +%s)" "$(( $(date +%s) - start ))" "$initial" "$(pidof gm-native)" >> evidence/soak.events
