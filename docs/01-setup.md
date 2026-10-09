# 01 — 준비: 7주차 커널 트리 다시 쓰기

7주차에 `~/linux-7.2.2` 를 빌드해서 그 커널로 부팅했다. 이번에도 **같은 트리**를 쓴다.
처음부터 다시 빌드할 필요는 없다 — 바뀐 파일만 다시 컴파일된다.

---

## 1. VM 스냅샷

시스템 콜 테이블을 잘못 고치면 부팅은 되어도 **모든 프로그램이 이상하게 동작**할 수 있다.
시작 전에 스냅샷을 찍는다.

## 2. 트리 상태 확인

```bash
cd ~/linux-7.2.2
git status --short          # 7주차 수정이 남아 있으면 보인다
git stash                   # 7주차 수정은 잠시 치워 둔다 (나중에 git stash pop)
# 또는 브랜치를 새로 판다
git switch -c lab04
```

7주차 패치를 남겨 둬도 이번 실습에는 지장이 없다. 다만 제출용 `partA.patch` 에
7주차 변경이 섞이지 않도록 **브랜치를 나누는 것을 권한다.**

7주차 빌드를 하지 않았다면 → 7주차 실습 자료의 `docs/01-kernel-build.md` 를 먼저 따라갈 것.

## 3. 재빌드 루프

이번 실습에서 커널을 고칠 때마다 반복하는 명령:

```bash
make -j$(nproc)
sudo make modules_install
sudo make install
sudo reboot
```

- 시스템 콜 테이블을 고치면 `asm/syscalls_64.h`, `asm/unistd_64.h` 가 **다시 생성**되고,
  이를 포함하는 파일이 함께 재컴파일된다. 첫 재빌드는 5~10분 걸릴 수 있다
- `modules_install` 을 해야 `/lib/modules/$(uname -r)/build` 가 새 트리를 가리켜
  Part C 의 모듈을 바로 빌드할 수 있다

## 4. 부팅 후 확인

```bash
uname -r                                       # 7.2.2 (또는 7.2.2-dirty)
grep -w hello /proc/kallsyms                    # __x64_sys_hello 등이 보이면 성공
sudo dmesg -w                                  # 이 창은 실습 내내 열어 둔다
```

`/proc/kallsyms` 에서 주소가 모두 `0000000000000000` 으로 보이면 `sudo` 를 붙인다.

## 5. 빌드 전에 미리 보기 — 생성되는 헤더

Part A를 마치고 빌드하면 다음 두 파일에 `hello` 가 생긴다. 직접 열어서 확인해 볼 것.

```bash
grep -n hello arch/x86/include/generated/uapi/asm/unistd_64.h   # #define __NR_hello 472
grep -n hello arch/x86/include/generated/asm/syscalls_64.h      # __SYSCALL(472, sys_hello)
```

두 번째 파일이 강의 슬라이드의 `x64_sys_call()` switch 문에 `#include` 되는 파일이다.
