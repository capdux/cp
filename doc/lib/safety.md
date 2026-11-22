# CP 标准库：并发竞态检测、硬件 CPU 特性与崩溃诊断子系统使用手册 (`cap:safety`)

> 参考来源：`/data/dev/bun/src/safety/safety.cp`、`/data/dev/bun/src/crash_handler/CPUFeatures.zig`、`handle_oom.zig`、`crash_handler.zig` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:safety` 是 CP 语言官方内置的**线程归属校验（`ThreadLock`）、可重入读写竞态检测（`CriticalSection`）、非托管容器分配器一致性守卫（`CheckedAllocator`）、AddressSanitizer 内存毒化接口（`asan`）、硬件 CPU ISA 特性探测（`CpuFeatures`）以及 OOM 策略守卫与崩溃诊断报告生成（`OomGuard` / `CrashReport`）**标准库。完整吸收自 Bun（`/data/dev/bun/src/safety/` 与 `/data/dev/bun/src/crash_handler/`），以接近零的运行时成本捕获并发竞态、跨分配器释放与异常崩溃上下文。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:safety 内存、并发与崩溃诊断防线                        │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│    线程归属与读写竞态检测   │    分配器与 OOM 策略守卫     │   CPU 特性探测与崩溃报告      │
│  • thread_id (getCurrent) │  • CheckedAllocator         │  • CpuFeatures.detect()      │
│  • ThreadLock (单线程锁)  │  • assertAllocatorEq        │    (CPUID + builtin 特征)    │
│  • CriticalSection        │  • OomGuard / OomPolicy     │  • CrashReport.capture()     │
│  • poison / unpoison      │    (checkAlloc / setHook)   │  • formatReport (零堆格式化)  │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 核心安全组件详解与代码示例

### 1. 线程归属锁与单线程契约验证 (`ThreadLock` & `thread_id`)

在事件循环或 Actor 内部，大量数据结构被设计为**单线程无锁访问**以追求极致吞吐。`ThreadLock` 并非阻塞式操作系统互斥锁（Mutex），而是一个**零系统调用的线程归属断言器**：
- 当线程调用 `lock()` 或 `lockOrAssert()` 时，记录当前操作系统线程 ID `std.Thread.getCurrentId()`；
- 若另一个线程试图并发访问并调用 `assertLocked()` 或 `lockOrAssert()`，立即触发确定性 `@panic` 报告跨线程违规访问。

```typescript
import {
    invalid,
    getCurrent,
    isCurrent,
    isValid,
    ThreadLock,
} from "cap:safety";

pub fn demoThreadLock() {
    const tid = getCurrent();
    const ok = isValid(tid) && isCurrent(tid);
    * = ok;

    let tlock = ThreadLock.initUnlocked();
    tlock.assertNotLocked();

    tlock.lock();
    tlock.assertLocked();
    tlock.lockOrAssert();

    tlock.unlock();
    tlock.assertNotLocked();
}
```

### 2. 可重入读写临界区竞态检测器 (`CriticalSection`)

`CriticalSection` 用于保护非线程安全容器免受**多线程并发读写冲突**或**不当的跨线程并发修改**：
- **`begin()` / `end()`（排他写临界区）**：支持同一线程内的嵌套重入（`owned_count += 1`）；
- **`beginReadOnly()` / `end()`（共享只读临界区）**：允许多个线程同时进入只读访问；若当前存在排他写则立即报错。

### 3. 非托管容器分配器一致性校验 (`CheckedAllocator`) & ASAN (`poison` / `unpoison`)

`CheckedAllocator` 记录首次绑定的 `vtable` 与 `ptr`，通过 `set(allocator)` 或 `assertEq(allocator)` 校验分配器实例一致性；`poison` / `unpoison` 为对象池空闲槽位提供 ASAN 毒化支持。

### 4. 硬件 CPU ISA 特性探测 (`CpuFeatures`)

吸收自 Bun `src/crash_handler/CPUFeatures.zig`：
- 在 `x86_64` 上结合编译期 `builtin.cpu.features` 与运行时原生 `CPUID` + `XGETBV` 指令精确探测 `sse2`、`sse4_2`、`avx`、`avx2`、`avx512f`、`aes`、`sha`、`popcnt`、`bmi1`、`bmi2`、`fma`；
- 在 `aarch64` 上探测 `neon`、`aes`、`sha2`、`crc32`；
- 提供 `hasSimd128()`、`hasSimd256()`、`hasHardwareAes()` 与零堆分配 `formatSummary(buf)`。

```typescript
import { CpuFeatures } from "cap:safety";

fn inspectCpu() {
    const cpu = CpuFeatures.detect();
    let buf: [256]u8 = undefined;
    const summary = cpu.formatSummary(&buf);
    if (cpu.hasSimd256() && cpu.hasHardwareAes()) {
        // 启用 AVX2 + AES-NI 高速通道
    }
    * = summary;
}
```

### 5. OOM 策略守卫与崩溃诊断报告 (`OomGuard` & `CrashReport`)

吸收自 Bun `src/crash_handler/handle_oom.zig` 与 `crash_handler.zig`：
- **`OomGuard`**：支持 `OomPolicy`（`Abort` / `Panic` / `SoftFail` / `Callback`），统计累计 OOM 次数、限制单次最大分配 `setMaxSingleAlloc`、挂载自定义通知钩子 `setHook`，通过 `checkAlloc` 执行受控内存分配；
- **`CrashReport`**：支持信号映射（`SIGSEGV`/`SIGABRT`/`SIGBUS`/`SIGFPE`/`SIGILL` 等）、采集当前 PID、CPU 特性摘要与自定义上下文，提供零堆分配的 `formatReport(buf)` 格式化输出。

```typescript
import std from "std";
import { OomGuard, OomPolicy, CrashReport } from "cap:safety";

fn demoCrashAndOom() {
    let guard = OomGuard.init(OomPolicy.SoftFail);
    guard.setMaxSingleAlloc(4096);

    if (buf := guard.checkAlloc(std.heap.page_allocator, 8192, "oversize")) {
        std.heap.page_allocator.free(buf);
    }

    const rep = CrashReport.capture(11, "invalid pointer read", "http_parser", 0x1000, guard.count());
    let report_buf: [2048]u8 = undefined;
    const text = rep.formatReport(&report_buf);
    * = text;
}
```

---

## 三、 与 Bun (`src/safety/` & `src/crash_handler/`) 对照表

| 安全防线能力 | Bun 上游模块 | CP 标准库 (`cap:safety`) | 运行时开销 |
| :--- | :--- | :--- | :--- |
| **线程 ID 标识与判别** | `bun.safety.thread_id` | `invalid`, `getCurrent`, `isCurrent`, `isValid` | 寄存器读取 TLS |
| **单线程所有权断言锁** | `bun.safety.ThreadLock` | `ThreadLock` (`lock`, `unlock`, `lockOrAssert`) | 单次 `u64` 比较 |
| **读写竞态检测临界区** | `bun.safety.CriticalSection` | `CriticalSection` (`begin`, `beginReadOnly`, `end`) | 原子计数器校验 |
| **分配器混用防护** | `bun.safety.CheckedAllocator` | `CheckedAllocator`, `assertAllocatorEq` | 2 个指针相等比较 |
| **ASAN 自定义池毒化** | `bun.asan.poison / unpoison` | `poison`, `unpoison`, `isPoisoned`, `registerRootRegion` | 非 ASAN 构建下为 0 |
| **CPU 硬件指令集探测** | `src/crash_handler/CPUFeatures.zig` | `CpuFeatures` (`detect`, `hasSimd128/256`, `formatSummary`) | 仅启动/崩溃时调用 |
| **OOM 守卫与崩溃报告** | `handle_oom.zig` / `crash_handler.zig` | `OomGuard`, `OomPolicy`, `CrashReport` | 零堆栈缓冲区格式化 |

---

## 四、 上游行为契约与安全边界注意事项

1. **`ThreadLock` 与 `CriticalSection` 不是阻塞互斥锁**：
   - 两者均为零系统调用的**并发契约断言器**，用于验证“应单线程独占”或“应读写分离”的高频容器在运行期未被意外跨线程误用；需要真实跨线程阻塞同步时请使用 `cap:sync` 的 `Mutex` / `RwLock`。
2. **`CrashReport.formatReport` 严格零堆分配**：
   - 在信号处理上下文（`SIGSEGV`/`SIGBUS`/`SIGABRT`）或 OOM 崩溃路径中，堆分配器本身可能已损坏或耗尽，因此 `formatReport(buf)` 全部写入调用方预分配栈缓冲，杜绝二次崩溃。
3. **`CpuFeatures.detect()` 与 `XGETBV` 校验**：
   - 在 `x86_64` 上不仅检查 `CPUID` 硬件位，同时结合操作系统 `XGETBV` 上下文保存支持，确保在旧内核或受限虚拟化环境下不会因错误启用 AVX/AVX2 触发 `SIGILL`。
