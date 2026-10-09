# Part A — 새 시스템 콜 `sys_hello` 추가

**배점 35점** · 고칠 곳 5군데 (새 파일 2 + 기존 파일 3)

```
long hello(const char __user *msg, unsigned int count);
```

| 반환 | 의미 |
|---|---|
| `0` | 성공. 메시지를 커널 로그에 출력하고, 타이머가 `count` 번 더 출력하도록 저장 |
| `-EFAULT` | `msg` 가 사용자 공간의 읽을 수 있는 주소가 아님 |
| `-EINVAL` | `count > HELLO_COUNT_MAX` (60) |

`count = 0` 이면 즉시 출력만 하고 타이머 쪽에는 아무것도 남기지 않는다.

---

## A-0. 수정할 곳 한눈에

| # | 파일 | 할 일 |
|---|---|---|
| 1 | `include/linux/hello.h` | **새 파일.** `kernel/hello.h` 를 그대로 복사 |
| 2 | `kernel/hello.c` | **새 파일.** `kernel/hello.c` 스켈레톤을 복사하고 TODO A-1~A-7 |
| 3 | `kernel/Makefile` | `obj-y += hello.o` 한 줄 |
| 4 | `arch/x86/entry/syscalls/syscall_64.tbl` | 472번 한 줄 |
| 5 | `include/linux/syscalls.h` | 프로토타입 한 줄 |

강의 슬라이드 *Final Steps in Binding a System Call (1/3)~(3/3)* 과 비교해 볼 것.
교재(2.6)의 `entry.S` 수정, `#define __NR_foo` 직접 작성은 **하지 않는다.**

```bash
cd ~/linux-7.2.2
cp <실습폴더>/kernel/hello.h include/linux/hello.h
cp <실습폴더>/kernel/hello.c kernel/hello.c
```

---

## A-1. 번호 정하기 — `syscall_64.tbl`

```bash
grep -v '^#' arch/x86/entry/syscalls/syscall_64.tbl | awk '$2!="x32"' | tail -3
```

```
470	common	listns			sys_listns
471	common	rseq_slice_yield	sys_rseq_slice_yield
```

v7.2.2 에서 `common` 의 마지막 번호는 **471**이다. 그 다음 줄에 추가한다.

```
472	common	hello			sys_hello
```

- 구분자는 **탭**이다. 공백 여러 개도 동작하지만 다른 줄과 맞춰 둘 것
- `512` 부터는 `x32` 전용 구간이다. 그 사이에 끼워 넣지 말 것
- 네 번째 칸에 `sys_hello` 라고 쓰지만 실제 진입점은 `__x64_sys_hello` 가 된다 (강의: Syscalls 2/2)

## A-2. 프로토타입 — `include/linux/syscalls.h`

`sys_listns` 선언 바로 아래에 추가한다.

```c
asmlinkage long sys_hello(const char __user *msg, unsigned int count);
```

`__user` 는 sparse 정적 분석기용 표시다. 이 포인터를 직접 역참조하면 `make C=1` 이 경고한다.

## A-3. 빌드에 넣기 — `kernel/Makefile`

```make
obj-y += hello.o
```

`obj-y` 는 **커널 이미지에 직접 링크**된다는 뜻이다. 시스템 콜은 모듈(`obj-m`)로 만들 수 없다 —
시스템 콜 테이블이 컴파일 시점에 고정되기 때문이다.

---

## A-4. 본체 — `kernel/hello.c` 의 TODO

### TODO A-1 — 매크로

```c
SYSCALL_DEFINE2(hello, const char __user *, msg, unsigned int, count)
```

타입과 이름 사이가 **쉼표**다. 인자 하나마다 두 칸을 쓴다.

### TODO A-2 — 인자 검사

`count > HELLO_COUNT_MAX` 이면 `-EINVAL`. **사용자에게서 온 값은 전부 의심한다** (강의: Verifying the Parameters).

### TODO A-3 — 문자열 복사

`strncpy_from_user()` 의 반환값은 세 가지다. **셋 다 처리해야 만점.**

```c
len = strncpy_from_user(buf, msg, sizeof(buf));
```

| `len` | 의미 | 할 일 |
|---|---|---|
| `< 0` | 잘못된 주소 | 그대로 `return len;` |
| `== sizeof(buf)` | 버퍼가 꽉 참 — NUL 이 **없다** | `buf[sizeof(buf) - 1] = '\0';` |
| 그 외 | 복사된 길이 (NUL 미포함) | 그대로 사용 |

### TODO A-4 — 로그

```c
pr_info("hello: pid %d (%s) says \"%s\", count=%u\n", current->pid, current->comm, buf, count);
```

`current` 는 **이 시스템 콜을 부른 프로세스**다 — 지금은 process context 이므로.
Part C 의 콜백에서 같은 `current` 를 찍어 보면 다른 것이 나온다 (보고서 Q5).

### TODO A-5 — 공유 상태 저장

```
  CPU 0
  ─────
  sys_hello()                               ← process context
    spin_lock(&hello_lock)        ┐
      ⚡ 타이머 인터럽트             │  같은 CPU
      tick_hello_fn()             │
        hello_take()              │
          spin_lock(&hello_lock)  ┘  ← 이미 내가 잡고 있다 → 영원히 대기
```

그래서 process context 쪽은 락을 잡는 동안 **로컬 인터럽트를 꺼야 한다.**

```c
spin_lock_irqsave(&hello_lock, flags);
/* strscpy(hello_state.msg, buf, sizeof(hello_state.msg)); remaining, pid 갱신 */
spin_unlock_irqrestore(&hello_lock, flags);
```

### TODO A-6 — `hello_take()`

`remaining > 0` 이면 하나 줄이고 메시지를 복사해 `true`. 락은 A-5와 같은 것을 잡는다.

### TODO A-7 — 내보내기

```c
EXPORT_SYMBOL_GPL(hello_take);
```

이게 없으면 Part C 의 `insmod` 가 `Unknown symbol hello_take` 로 실패한다.

---

## A-5. 빌드와 확인

```bash
make -j$(nproc) 2>&1 | grep -E 'hello|error|warning'     # 경고 0 이 목표
sudo make modules_install && sudo make install && sudo reboot
```

부팅 후:

```bash
grep -n hello arch/x86/include/generated/uapi/asm/unistd_64.h
sudo grep -w -e __x64_sys_hello -e hello_take /proc/kallsyms
```

```
#define __NR_hello 472
ffffffff8xxxxxxx T __x64_sys_hello
ffffffff8xxxxxxx T hello_take
```

아직 호출할 프로그램이 없다 → Part B 로.

## A-6. 패치 만들기

```bash
git add -N kernel/hello.c include/linux/hello.h     # 새 파일을 diff 에 포함
git diff > ~/partA.patch
git diff --stat
```

```
 arch/x86/entry/syscalls/syscall_64.tbl |  1 +
 include/linux/hello.h                  | 15 ++++
 include/linux/syscalls.h               |  2 +
 kernel/Makefile                        |  1 +
 kernel/hello.c                         | 94 ++++++++++
```
