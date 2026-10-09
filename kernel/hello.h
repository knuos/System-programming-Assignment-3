/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/hello.h — sys_hello 와 타이머 모듈이 공유하는 선언
 * 수정하지 말고 그대로 include/linux/hello.h 로 복사한다.
 */
#ifndef _LINUX_HELLO_H
#define _LINUX_HELLO_H

#include <linux/types.h>

#define HELLO_MSG_MAX	64	/* NUL 포함 최대 길이 */
#define HELLO_COUNT_MAX	60	/* 타이머 출력 최대 횟수 — 로그 폭주 방지 */

bool hello_take(char *buf, size_t len, unsigned int *left, pid_t *pid);

#endif /* _LINUX_HELLO_H */
