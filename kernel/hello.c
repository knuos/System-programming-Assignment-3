// SPDX-License-Identifier: GPL-2.0
/*
 * kernel/hello.c — 실습용 시스템 콜 sys_hello  (스켈레톤)
 *
 * 이 파일을 커널 소스의 kernel/hello.c 로 복사한 뒤 TODO 를 채운다.
 *
 *   long hello(const char __user *msg, unsigned int count);
 *
 *   1) 사용자 문자열 msg 를 커널로 안전하게 복사해서 커널 로그에 출력한다.
 *   2) 그 문자열과 count 를 공유 상태(hello_state)에 저장한다.
 *   3) 타이머 인터럽트 쪽(module/tick_hello.ko)이 hello_take() 로 하나씩 꺼내 간다.
 *
 *   반환값: 0 성공, -EFAULT 잘못된 포인터, -EINVAL count 범위 초과
 */
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/hello.h>

/*
 * 공유 상태 — process context(시스템 콜)와 hardirq context(hrtimer 콜백)가
 * 같이 접근한다.  질문: 왜 mutex 가 아니라 spinlock 이어야 하는가? (보고서 Q2)
 */
static struct {			/* __maybe_unused: TODO 를 채우기 전에도 빌드되도록 */
	char		msg[HELLO_MSG_MAX];
	unsigned int	remaining;	/* 타이머가 앞으로 출력할 횟수 */
	pid_t		pid;		/* 마지막으로 요청한 프로세스 */
} hello_state __maybe_unused;

static __maybe_unused DEFINE_SPINLOCK(hello_lock);

/*
 * TODO A-1: 인자 두 개짜리 시스템 콜을 정의한다.
 *   - 매크로: SYSCALL_DEFINEn(이름, 타입1, 인자1, 타입2, 인자2)
 *   - 이름은 "hello"  → 빌드 후 실제 심볼은 __x64_sys_hello 가 된다
 */
/* SYSCALL_DEFINE?( ... ) */
long __todo_sys_hello(const char __user *msg, unsigned int count);	/* ← 이 줄도 지울 것 */
long __todo_sys_hello(const char __user *msg, unsigned int count)	/* ← 이 줄을 지우고 위 매크로로 */
{
	char buf[HELLO_MSG_MAX];
	unsigned long flags;
	long len;

	/* TODO A-2: count 가 HELLO_COUNT_MAX 보다 크면 -EINVAL */

	/*
	 * TODO A-3: msg 를 buf 로 복사한다.
	 *   - *msg 를 직접 읽으면 안 된다 (강의 슬라이드: SMAP)
	 *   - strncpy_from_user(dst, src, count) 의 반환값:
	 *       < 0           → 오류 (-EFAULT). 그대로 반환
	 *       == count      → 버퍼가 꽉 참. NUL 이 없으므로 마지막 바이트를 '\0' 으로
	 *       그 외         → 복사된 길이 (NUL 미포함)
	 */
	len = 0;

	/* TODO A-4: pr_info() 로 pid, 프로세스 이름(current->comm), 메시지, count 출력 */

	/*
	 * TODO A-5: hello_lock 을 잡고 hello_state 의 세 필드를 갱신한다.
	 *   - 이 코드는 process context 다. 락을 잡은 채로 같은 CPU 에 타이머 인터럽트가
	 *     들어와 hello_take() 가 같은 락을 잡으려 하면?  → irqsave 버전을 쓸 것
	 *   - 문자열 복사는 strscpy(dst, src, sizeof(dst))
	 */
	(void)flags;
	(void)len;
	(void)buf;

	return 0;
}

/*
 * hello_take — 타이머 인터럽트 쪽에서 호출한다 (hardirq context).
 *   남은 횟수가 있으면 메시지를 buf 에 복사하고 횟수를 1 줄인 뒤 true.
 *   sleep 할 수 있는 함수(kmalloc(GFP_KERNEL), mutex_lock, copy_*_user ...)는 금지.
 */
bool hello_take(char *buf, size_t len, unsigned int *left, pid_t *pid)
{
	unsigned long flags;
	bool ret = false;

	/* TODO A-6: hello_lock 을 잡고, remaining > 0 이면
	 *           remaining--, buf/left/pid 채우고 ret = true */
	(void)flags;

	return ret;
}
/* TODO A-7: 모듈이 이 함수를 쓸 수 있도록 GPL 심볼로 내보낸다 */
