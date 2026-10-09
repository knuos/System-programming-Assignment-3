// SPDX-License-Identifier: GPL-2.0
/*
 * tick_hello.c — 타이머 인터럽트마다 sys_hello 메시지를 출력하는 모듈 (스켈레톤)
 *
 *   local APIC timer IRQ
 *     → sysvec_apic_timer_interrupt()        arch/x86/kernel/apic/apic.c
 *     → local_apic_timer_interrupt()
 *     → evt->event_handler = hrtimer_interrupt()   kernel/time/hrtimer.c
 *     → __hrtimer_run_queues()
 *     → tick_hello_fn()   ← 이 파일. 스케줄러 tick(tick_nohz_handler)도 같은 길로 온다
 *
 *   sudo insmod tick_hello.ko period_ms=1000
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/smp.h>
#include <linux/preempt.h>
#include <linux/hello.h>

static unsigned int period_ms = 1000;
module_param(period_ms, uint, 0444);
MODULE_PARM_DESC(period_ms, "timer period in ms (10..10000, default 1000)");

static struct hrtimer tick_timer __maybe_unused;	/* __maybe_unused: TODO 전에도 빌드되도록 */
static ktime_t period;
static unsigned long fired;	/* 콜백이 실행된 총 횟수 */
static unsigned long printed;	/* 실제로 메시지를 출력한 횟수 */

static enum hrtimer_restart __maybe_unused tick_hello_fn(struct hrtimer *t)
{
	char msg[HELLO_MSG_MAX];
	unsigned int left;
	pid_t pid;

	fired++;

	/*
	 * TODO C-1: hello_take() 로 메시지를 하나 꺼내 왔으면 printed++ 하고
	 *   pr_info("tick_hello: [cpu%d hardirq=%d] \"%s\" from pid %d (left %u)\n", ...)
	 *   - CPU 번호: smp_processor_id()
	 *   - hardirq 여부: in_hardirq()     (강의 슬라이드: in_irq() 는 삭제됨)
	 *
	 * 여기는 hardirq context — sleep 금지, 짧게 끝낼 것
	 */
	(void)msg; (void)left; (void)pid;

	/*
	 * TODO C-2: 다음 만료 시각을 period 만큼 뒤로 미루고 HRTIMER_RESTART 반환
	 *   hrtimer_forward_now(t, period)
	 */
	return HRTIMER_NORESTART;
}

static int __init tick_hello_init(void)
{
	if (period_ms < 10 || period_ms > 10000)
		return -EINVAL;

	period = ms_to_ktime(period_ms);

	/*
	 * TODO C-3: 타이머 초기화와 시작
	 *   hrtimer_setup(&tick_timer, 콜백, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
	 *   hrtimer_start(&tick_timer, period, HRTIMER_MODE_REL_HARD);
	 *   (_HARD: PREEMPT_RT 에서도 softirq 스레드로 밀리지 않고 hardirq 에서 실행)
	 *   ※ 옛 예제의 hrtimer_init() + t.function = ... 방식은 v7.2 에 없다 (hrtimer_setup 으로 대체)
	 */

	pr_info("tick_hello: loaded, period %u ms\n", period_ms);
	return 0;
}

static void __exit tick_hello_exit(void)
{
	/* TODO C-4: 타이머 취소. 이걸 빼먹고 rmmod 하면 무슨 일이 생기나? (보고서 Q4) */

	pr_info("tick_hello: unloaded, fired %lu times, printed %lu messages\n",
		fired, printed);
}

module_init(tick_hello_init);
module_exit(tick_hello_exit);

MODULE_LICENSE("GPL");		/* EXPORT_SYMBOL_GPL(hello_take) 를 쓰려면 필수 */
MODULE_AUTHOR("26F Advanced System Programming, KNU");
MODULE_DESCRIPTION("print sys_hello messages from the timer interrupt");
