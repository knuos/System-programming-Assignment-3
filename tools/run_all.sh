#!/usr/bin/env bash
# run_all.sh — Part A~C 를 순서대로 실행하고 결과를 out/ 에 남긴다.
#   sudo ./run_all.sh
set -uo pipefail
cd "$(dirname "$0")/.."
OUT=tools/out; mkdir -p "$OUT"
log() { printf '\n==== %s ====\n' "$*" | tee -a "$OUT/run.log"; }

: > "$OUT/run.log"
log "uname -r"; uname -r | tee -a "$OUT/run.log"

log "Part B: user program"
make -s -C user || exit 1
dmesg -C
./user/hello_user "Hello from user space" 3 | tee -a "$OUT/run.log"
./user/hello_user --bad                    | tee -a "$OUT/run.log"
./user/hello_user --big                    | tee -a "$OUT/run.log"
dmesg | tee -a "$OUT/run.log"

log "strace"
if command -v strace >/dev/null; then
	# 알 수 없는 번호는 strace 가 syscall_0x1d8(...) 형태로 보여 준다 (472 = 0x1d8)
	strace ./user/hello_user "strace test" 0 2>&1 | grep -E "syscall_0x|^hello\(" | tee -a "$OUT/run.log"
fi

log "Part C: tick_hello module"
make -s -C module || exit 1
rmmod tick_hello 2>/dev/null
dmesg -C
insmod module/tick_hello.ko period_ms=1000 || exit 1
./user/hello_user "tick tock" 5 | tee -a "$OUT/run.log"
./tools/loc_count.sh 7 | tee -a "$OUT/run.log"
rmmod tick_hello
dmesg | tee -a "$OUT/run.log"

echo; echo "결과: $OUT/run.log"
