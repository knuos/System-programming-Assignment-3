# Part B — `sys_hello` 를 부르는 사용자 프로그램

**배점 15점** · 고칠 파일: `user/hello_user.c`

glibc 는 `hello()` 라는 함수를 모른다. 그래서 **번호로 직접** 부른다.

```c
#include <unistd.h>
#include <sys/syscall.h>

long syscall(long number, ...);     /* glibc 의 범용 래퍼 — man 2 syscall */
```

`syscall()` 은 번호를 `rax` 에, 인자를 `rdi, rsi, rdx, r10, r8, r9` 에 넣고 `syscall` 명령을 실행한다.
커널이 음수(`-EFAULT` 등)를 돌려주면 `-1` 을 반환하고 `errno` 에 양수를 넣는다.

---

## TODO B-1 — 번호

새 커널의 헤더를 설치하지 않았다면 `<sys/syscall.h>` 에 `__NR_hello` 가 없다. 직접 정의한다.

```c
#ifndef __NR_hello
#define __NR_hello 472
#endif
```

> 설치된 헤더로 확인하는 법: `sudo make headers_install INSTALL_HDR_PATH=/usr` 를 했다면
> `grep __NR_hello /usr/include/asm/unistd_64.h` 가 보인다. 필수는 아니다.

## TODO B-2 — 호출

```c
return syscall(__NR_hello, msg, count);
```

## TODO B-3 — 잘못된 포인터

```c
report("hello((char *)0xffffffff81000000, 3)", do_hello((const char *)0xffffffff81000000UL, 3));
report("hello(NULL, 3)", do_hello(NULL, 3));
```

`0xffffffff81000000` 은 x86-64 커널 텍스트가 보통 시작하는 근처의 주소다.
**커널이 이것을 그냥 읽어 준다면** 사용자가 커널 메모리를 마음대로 볼 수 있다는 뜻이다.

---

## B-4. 실행

```bash
make -C user
./user/hello_user "Hello from user space" 3
./user/hello_user --bad
./user/hello_user --big
```

기대 결과:

```
pid 1234: syscall(472, "Hello from user space", 3)
[ok]   hello → 0
[fail] hello((char *)0xffffffff81000000, 3) → -1, errno=14 (Bad address)
[fail] hello(NULL, 3) → -1, errno=14 (Bad address)
[fail] hello("x", 100000) → -1, errno=22 (Invalid argument)
```

`dmesg -w` 창:

```
hello: pid 1234 (hello_user) says "Hello from user space", count=3
```

`--bad` 일 때는 dmesg 에 **아무것도 찍히지 않아야** 한다 (복사 단계에서 이미 돌아갔으므로).

## B-5. strace 로 경계 보기

```bash
strace ./user/hello_user "strace test" 0 2>&1 | grep syscall_
```

```
syscall_0x1d8(0x7ffd..., 0, 0x..., 0x..., 0, 0) = 0
```

- strace 는 472(`0x1d8`)번의 이름을 모른다 → 번호로 보여 준다
- 인자가 **6개** 보인다. strace 는 인자 개수를 모르므로 레지스터 6개를 전부 읽는다
- 첫 번째가 문자열 주소, 두 번째가 `count` 다. 각각 어느 레지스터인가? (보고서 Q1)

stock 커널(7주차 이전 커널)로 부팅해서 같은 명령을 실행하면 `= -1 ENOSYS` 가 나온다.
번호가 테이블에 없으면 `x64_sys_call()` switch 의 `default:` → `__x64_sys_ni_syscall()` 로 간다.
