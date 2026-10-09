# Part C — 타이머 인터럽트마다 메시지 출력

**배점 30점** · 고칠 파일: `module/tick_hello.c` (커널 재빌드 없음)

---

## C-0. 왜 `request_irq()` 가 아닌가

교재의 RTC 예제처럼 `request_irq(타이머 IRQ, handler, IRQF_SHARED, ...)` 를 하고 싶어진다.
지금 x86 에서는 그렇게 할 수 없다.

```bash
grep -E '^ *0:|LOC:' /proc/interrupts
```

```
  0:         45          0   IO-APIC   2-edge      timer
LOC:     812345     790221   Local timer interrupts
```

- `0:` (PIT) 은 부팅 초기에만 쓰이고 거의 늘지 않는다
- 실제 tick 은 각 CPU 의 **local APIC timer** 가 만든다 → `LOC:` 줄. 이것은 IRQ 번호가 없는 **시스템 벡터**다
- 드라이버가 handler 를 붙일 수 있는 자리가 없다

대신 커널은 local APIC timer 위에 **hrtimer** 라는 계층을 둔다.
스케줄러 tick 자체도 hrtimer 하나다 (`tick_nohz_handler`, `HRTIMER_MODE_ABS_HARD`).
우리도 hrtimer 를 하나 걸면 **같은 인터럽트, 같은 hardirq context** 에서 실행된다.

```
local APIC timer 만료
  → sysvec_apic_timer_interrupt()          arch/x86/kernel/apic/apic.c
  → local_apic_timer_interrupt()
  → evt->event_handler(evt)  = hrtimer_interrupt()     kernel/time/hrtimer.c
  → __hrtimer_run_queues()
      ├→ tick_nohz_handler()  → update_process_times() → sched_tick()   (스케줄러 tick)
      └→ tick_hello_fn()      ← 우리 콜백
```

Elixir 에서 위 함수들을 차례로 따라가 볼 것.

---

## C-1. TODO

### TODO C-3 — 초기화와 시작 (먼저 이것부터)

```c
hrtimer_setup(&tick_timer, tick_hello_fn, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
hrtimer_start(&tick_timer, period, HRTIMER_MODE_REL_HARD);
```

> 인터넷의 예제 대부분은 `hrtimer_init(&t, ...); t.function = fn;` 이다.
> v7.2 에는 `hrtimer_init()` 이 **없다** — 컴파일 오류가 나면 이것 때문이다.
> Linux Kernel Labs 의 *Deferred work* 실습도 v5.10 기준이라 API 가 다르다.

### TODO C-2 — 다시 걸기

```c
hrtimer_forward_now(t, period);
return HRTIMER_RESTART;
```

`hrtimer_forward_now()` 는 만료 시각을 **이전 만료 시각 + period** 로 옮긴다.
콜백이 늦게 실행되어도 오차가 쌓이지 않는다.

### TODO C-1 — 출력

```c
if (hello_take(msg, sizeof(msg), &left, &pid)) {
	printed++;
	pr_info("tick_hello: [cpu%d hardirq=%d] \"%s\" from pid %d (left %u)\n",
		smp_processor_id(), in_hardirq() ? 1 : 0, msg, pid, left);
}
```

강의 슬라이드 *Status of the Interrupt System*: `in_irq()` 는 삭제되었다. `in_hardirq()` 를 쓴다.

### TODO C-4 — 정리

```c
hrtimer_cancel(&tick_timer);
```

콜백이 지금 다른 CPU 에서 실행 중이면 **끝날 때까지 기다렸다가** 취소한다.

---

## C-2. 실행

```bash
make -C module
sudo insmod module/tick_hello.ko                 # period_ms=1000 기본
./user/hello_user "tick tock" 5
sleep 6
sudo rmmod tick_hello
```

`dmesg -w` 창 (시각은 예시):

```
[  812.001] tick_hello: loaded, period 1000 ms
[  815.442] hello: pid 2001 (hello_user) says "tick tock", count=5
[  816.001] tick_hello: [cpu1 hardirq=1] "tick tock" from pid 2001 (left 4)
[  817.001] tick_hello: [cpu1 hardirq=1] "tick tock" from pid 2001 (left 3)
[  818.001] tick_hello: [cpu1 hardirq=1] "tick tock" from pid 2001 (left 2)
[  819.001] tick_hello: [cpu1 hardirq=1] "tick tock" from pid 2001 (left 1)
[  820.001] tick_hello: [cpu1 hardirq=1] "tick tock" from pid 2001 (left 0)
[  824.310] tick_hello: unloaded, fired 12 times, printed 5 messages
```

확인할 것:

- `hardirq=1` — 정말로 인터럽트 문맥에서 실행되었다
- 간격이 1.000 초에 가깝다 — `hrtimer_forward_now()` 의 효과
- `fired` 는 모듈이 올라가 있던 시간(초)과 같고, `printed` 는 `count` 와 같다
- 첫 tick 메시지는 시스템 콜 직후가 아니라 **다음 만료 시각**에 나온다

`period_ms=100` 으로 다시 넣고 `count=20` 을 줘 보자. 2초 동안 20줄.

---

## C-3. 측정 — LOC 와 비교 (보고서 Q3)

```bash
./tools/loc_count.sh 10                 # 유휴 상태
for i in $(seq $(nproc)); do yes > /dev/null & done
./tools/loc_count.sh 10                 # 부하 상태
killall yes
```

`CONFIG_HZ` 가 250 이면 tick 은 초당 250번이어야 한다. 유휴 CPU 에서 LOC 가 그보다 훨씬 적다면
**왜** 그런지 `CONFIG_NO_HZ_IDLE` 로 설명할 것. 우리 hrtimer 는 1초에 한 번이지만 LOC 를 늘린다 —
CPU 가 쉬고 있어도 우리 타이머 때문에 깨어나야 하기 때문이다.

---

## C-4. 해 보면 좋은 실험 (스냅샷 후, 선택)

| 실험 | 예상 | 무엇을 배우나 |
|---|---|---|
| 콜백에 `msleep(1);` 추가 | `BUG: scheduling while atomic` | hardirq 에서 sleep 금지 |
| `hello_take()` 의 락을 `spin_lock()` 으로, `sys_hello()` 도 `spin_lock()` 으로 | 드물게 hard lockup (lockdep 켜져 있으면 경고) | Q2 의 시나리오 |
| `hrtimer_cancel()` 제거 후 `rmmod` | 다음 만료 때 oops | Q4 |

**해 봤다면 dmesg 를 보고서에 첨부하면 가산점 대상이다.** 반드시 스냅샷을 찍고 할 것.
