# 실습 — System Calls & Interrupts

**26F 시스템프로그래밍특론 (Advanced System Programming)**
경북대학교 전자공학부 대학원 · 담당: 이경운 (kwlee87@knu.ac.kr)

---

## 1. 실습 개요

- **시스템 콜** — 사용자 공간과 커널 간 인터페이스 (process context)
- **인터럽트** — 하드웨어와 커널 간 인터페이스 (interrupt context, sleep 불가)

이번 실습에서는 이 두 인터페이스를 **하나의 데이터로 연결**한다.

```
 사용자 프로그램                 커널
 ─────────────                  ───────────────────────────────────────────
 hello_user "tick tock" 5
   └─ syscall(472, msg, 5) ──▶  sys_hello()            [process context]
                                  ├ strncpy_from_user()  사용자 문자열 복사
                                  ├ pr_info()            "hello: pid ... says ..."
                                  └ hello_state 저장     { msg, remaining = 5 }
                                         ▲ spinlock
 local APIC timer ──── IRQ ───▶  hrtimer_interrupt()   [hardirq context]
                                  └ tick_hello_fn()      tick_hello.ko
                                      └ hello_take()     remaining-- → pr_info()
                                                         "tick_hello: ... (left 4)"
```

| Part | 내용 | 고치는 곳 | 배점 |
|---|---|---|---|
| **A** | 새 시스템 콜 `sys_hello` 추가 | 커널 소스 5곳 (재빌드) | 35 |
| **B** | `sys_hello` 를 부르는 사용자 프로그램 | `user/hello_user.c` | 15 |
| **C** | 타이머 인터럽트마다 메시지를 출력하는 모듈 | `module/tick_hello.c` | 30 |
| 보고서 | 질문 Q1~Q5 + 측정 결과 | — | 20 |
| 심화 | 문서 끝의 심화 과제 중 택 1 | — | +10 |

**A → B → C 순서로 진행할 것.** C의 모듈은 A에서 커널이 내보낸 `hello_take()` 를 쓴다.
A가 안 된 커널에서는 `insmod` 가 `Unknown symbol hello_take` 로 실패한다.

---

## 2. 진행 순서

```
docs/01-setup.md          ← 먼저. 7주차에 만든 커널 트리를 다시 쓴다
docs/02-partA-syscall.md  ← Part A  (커널 수정 + 재빌드 + 재부팅)
docs/03-partB-user.md     ← Part B
docs/04-partC-timer.md    ← Part C
```

---

## 3. 디렉터리 구성

```
lab04-syscall-interrupt/
├── README.md               ← 지금 읽는 파일 (과제 명세)
├── docs/                   단계별 안내와 TODO 설명
├── kernel/
│   ├── hello.c             → 커널 소스의 kernel/hello.c 로 복사 (TODO A-1~A-7)
│   └── hello.h             → include/linux/hello.h 로 복사 (수정 불필요)
├── user/
│   ├── hello_user.c        TODO B-1~B-3
│   └── Makefile
├── module/
│   ├── tick_hello.c        TODO C-1~C-4
│   └── Makefile
└── tools/
    ├── loc_count.sh        /proc/interrupts 의 LOC(로컬 타이머 인터럽트) 측정
    └── run_all.sh          B·C 를 한 번에 실행하고 로그를 tools/out/ 에 저장
```

---

## 4. 빠른 시작

```bash
# 1) Part A — 커널 수정 (docs/02), 재빌드, 재부팅
cp kernel/hello.c ~/linux-7.2.2/kernel/hello.c
cp kernel/hello.h ~/linux-7.2.2/include/linux/hello.h
#    + syscall_64.tbl / syscalls.h / kernel/Makefile 수정

# 2) 다른 터미널에서 커널 로그를 계속 본다
sudo dmesg -w

# 3) Part B, C
make -C user && ./user/hello_user "Hello" 3
make -C module && sudo insmod module/tick_hello.ko
./user/hello_user "tick tock" 5          # 1초마다 tick_hello 메시지 5줄
sudo rmmod tick_hello

# 4) 제출용 로그 한 번에
sudo ./tools/run_all.sh
```

> `printk()` 출력은 **사용자 터미널에 나오지 않는다.** 커널 로그 버퍼로 간다.
> 반드시 `dmesg -w` (또는 `journalctl -kf`) 창을 옆에 띄워 두고 진행할 것.

---

## 5. 제출물

`<학번>_<이름>_lab04.zip` 으로 LMS에 제출한다.

```
<학번>_<이름>_lab04/
├── partA.patch             cd ~/linux-7.2.2 && git add -N kernel/hello.c include/linux/hello.h && git diff
├── hello_user.c
├── tick_hello.c
├── out/run.log             sudo ./tools/run_all.sh 결과 전체
├── report.pdf              보고서 (4~8쪽)
└── screenshots/            uname -r, dmesg -w 창, strace 결과
```

`git add -N` 을 해야 새로 만든 파일(`hello.c`, `hello.h`)이 `git diff` 에 들어간다.
커널 소스 트리 전체는 **제출하지 말 것.**

### 보고서 질문

| | 질문 | 관련 Part |
|---|---|---|
| **Q1** | `./hello_user --bad` 의 결과를 보이고, 커널이 왜 죽지 않았는지 설명하라. `strncpy_from_user()` 대신 `msg[0]` 을 직접 읽었다면 어떻게 되는가? (강의: SMAP, exception table) `strace` 출력의 `syscall_0x1d8(...)` 에는 인자가 6개 보인다. `msg`, `count` 는 어느 레지스터로 전달되었나? | A, B |
| **Q2** | `hello_lock` 이 mutex 가 아니라 spinlock 인 이유, 그리고 `sys_hello()` 에서 `spin_lock()` 이 아니라 `spin_lock_irqsave()` 를 써야 하는 이유를 **CPU 하나에서 일어나는 시나리오**로 설명하라 | A, C |
| **Q3** | `tools/loc_count.sh 10` 으로 측정한 LOC 증가율(초당)을 `CONFIG_HZ` 값과 비교하라. 차이가 나는 이유는? (`CONFIG_NO_HZ_IDLE`) 부하를 걸었을 때(`yes > /dev/null &` ×vCPU 수)는 어떻게 변하는가? | C |
| **Q4** | `tick_hello_exit()` 에서 `hrtimer_cancel()` 을 빼고 `rmmod` 하면 무슨 일이 생기는지 설명하라. (**실제로 해 보려면 스냅샷 후에.** 해 봤다면 dmesg 를 첨부) | C |
| **Q5** | 콜백 안에서 `current->comm` 과 `current->pid` 를 함께 출력해 보라. 어떤 이름이 주로 나오는가? `sys_hello()` 안의 `current` 와 의미가 어떻게 다른가? (강의: Interrupt Context) | A, C |

---

## 6. 자주 막히는 지점

| 증상 | 원인과 해결 |
|---|---|
| `errno=38 (Function not implemented)` | 새 커널로 부팅하지 않았다. `uname -r` 확인, `syscall_64.tbl` 에 472 줄이 있는지 확인 |
| 빌드 중 `implicit declaration of function 'hello_take'` | `include/linux/hello.h` 를 복사하지 않았다 |
| 링크 단계 `undefined reference to '__x64_sys_hello'` | `kernel/Makefile` 에 `obj-y += hello.o` 를 안 넣었거나 `SYSCALL_DEFINE2` 이름이 다르다 |
| `insmod: Unknown symbol hello_take` | 모듈을 stock 커널에서 넣었거나 `EXPORT_SYMBOL_GPL` 을 빠뜨렸다 |
| `insmod` 는 됐는데 tick 메시지가 없다 | `./hello_user` 를 아직 안 불렀다 (remaining = 0) / `HRTIMER_RESTART` 를 반환하지 않는다 |
| 메시지가 한 번에 쏟아진다 | `hrtimer_forward_now()` 를 빼먹어 만료 시각이 그대로다 |
| `dmesg: read kernel buffer failed: Operation not permitted` | `sudo dmesg` |
| `make -C module` 이 `No such file or directory: .../build` | 새 커널을 `make modules_install` 로 설치하지 않았다. `make KDIR=~/linux-7.2.2` 로 직접 지정 |

---

## 7. 안전 수칙

- **반드시 VM에서.** 실습 시작 전 스냅샷을 찍을 것
- 콜백(hardirq)에서 `msleep`, `mutex_lock`, `kmalloc(GFP_KERNEL)`, `copy_to_user` **금지**
- `period_ms` 하한(10 ms)과 `HELLO_COUNT_MAX`(60)를 **지우지 말 것** — 매 tick 출력은 콘솔을 멈추게 한다
- `rmmod` 전에 타이머 취소 — 해제된 코드로 점프하면 즉시 oops

---

## 8. 참고 자료

- Robert Love, *Linux Kernel Development*, 3rd ed. — Ch.5 System Calls, Ch.7 Interrupts, Ch.11 Timers
- 강의 슬라이드 — `5. System Calls (Linux 7.2).pptx`
- Elixir v7.2.2
  - `arch/x86/entry/syscalls/syscall_64.tbl`, `include/linux/syscalls.h`
  - `strncpy_from_user()` → `lib/strncpy_from_user.c`
  - `sysvec_apic_timer_interrupt()` → `arch/x86/kernel/apic/apic.c`
  - `hrtimer_interrupt()`, `hrtimer_setup()` → `kernel/time/hrtimer.c`
  - `tick_nohz_handler()` → `kernel/time/tick-sched.c`
- Linux Kernel Labs
  - System Calls — <https://linux-kernel-labs.github.io/refs/heads/master/lectures/syscalls.html>
  - Interrupts — <https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html>
  - Deferred work (lab, 커널 타이머) — <https://linux-kernel-labs.github.io/refs/heads/master/labs/deferred_work.html>
- `man 2 syscall`, `man 1 strace`, `Documentation/process/adding-syscalls.rst`

---
