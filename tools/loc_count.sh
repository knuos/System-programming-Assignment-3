#!/usr/bin/env bash
# loc_count.sh — /proc/interrupts 의 LOC(Local timer interrupts) 를 N 초 동안 센다.
#   ./loc_count.sh [초=10]
# CPU 별 증가량과 초당 평균을 출력한다. tick_hello 의 콜백 횟수와 비교할 것 (보고서 Q3).
set -euo pipefail
SEC=${1:-10}

snap() { awk '$1=="LOC:"{for(i=2;i<=NF;i++) if($i ~ /^[0-9]+$/) printf "%s ", $i; print ""}' /proc/interrupts; }

read -ra A <<< "$(snap)"
sleep "$SEC"
read -ra B <<< "$(snap)"

total=0
for i in "${!A[@]}"; do
	d=$(( B[i] - A[i] ))
	total=$(( total + d ))
	printf "CPU%-3d LOC +%-8d (%d/s)\n" "$i" "$d" $(( d / SEC ))
done
printf "합계   LOC +%-8d (%d/s)   — %d 초 동안\n" "$total" $(( total / SEC )) "$SEC"
grep -E '^CONFIG_HZ=|^CONFIG_NO_HZ' "/boot/config-$(uname -r)" 2>/dev/null || true
