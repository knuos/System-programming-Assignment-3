/*
 * hello_user.c — sys_hello 를 호출하는 사용자 프로그램 (스켈레톤)
 *
 *   ./hello_user "메시지" [count]    정상 호출
 *   ./hello_user --bad               잘못된 포인터 → EFAULT 확인
 *   ./hello_user --big               count 범위 초과 → EINVAL 확인
 *
 * glibc 에 hello() 래퍼가 없다. 그럼 어떻게 부르나?  (강의 슬라이드 Accessing from User-Space)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

/* TODO B-1: syscall_64.tbl 에 넣은 번호로 __NR_hello 를 정의 (이미 정의돼 있으면 그대로) */
#ifndef __NR_hello
#define __NR_hello -1
#endif

static long do_hello(const char *msg, unsigned int count)
{
	/* TODO B-2: syscall(2) 로 __NR_hello 를 호출하고 그 반환값을 돌려준다 */
	(void)msg; (void)count;
	errno = ENOSYS;
	return -1;
}

static void report(const char *what, long ret)
{
	if (ret == 0)
		printf("[ok]   %s → 0\n", what);
	else
		printf("[fail] %s → -1, errno=%d (%s)\n", what, errno, strerror(errno));
}

int main(int argc, char *argv[])
{
	if (argc >= 2 && strcmp(argv[1], "--bad") == 0) {
		/* TODO B-3: 잘못된 포인터 두 가지로 호출해 보고 결과를 report() 로 출력
		 *   (a) 커널 주소  (const char *)0xffffffff81000000UL
		 *   (b) NULL
		 * 커널이 죽지 않고 EFAULT 를 돌려줘야 정답이다 */
		return 0;
	}
	if (argc >= 2 && strcmp(argv[1], "--big") == 0) {
		report("hello(\"x\", 100000)", do_hello("x", 100000));
		return 0;
	}

	const char *msg = argc >= 2 ? argv[1] : "Hello from user space";
	unsigned int count = argc >= 3 ? (unsigned int)strtoul(argv[2], NULL, 10) : 5;

	printf("pid %d: syscall(%d, \"%s\", %u)\n", getpid(), __NR_hello, msg, count);
	long ret = do_hello(msg, count);
	report("hello", ret);

	if (ret < 0 && errno == ENOSYS)
		printf("       → 이 커널에는 %d 번 시스템 콜이 없다. 새 커널로 부팅했는지 uname -r 로 확인\n",
		       __NR_hello);
	return ret == 0 ? 0 : 1;
}
