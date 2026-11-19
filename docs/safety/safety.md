# CP 标准库：并发竞态检测、分配器一致性与内存安全子系统使用手册 (`cap:safety`)

## 一、 架构总览与设计哲学

`cap:safety` 是 CP 语言官方内置的**线程归属校验（`ThreadLock`）、可重入读写竞态检测（`CriticalSection`）、非托管容器分配器一致性守卫（`CheckedAllocator`）以及 AddressSanitizer 内存毒化接口（`asan`）**标准库。完整吸收自 Bun（`/data/dev/bun/src/safety/`），旨在以接近零的运行时成本，在开发与测试阶段第一时间捕获跨线程非法共享、迭代器失效重入与跨分配器释放（Cross-Allocator Free）等隐蔽的系统级内存错误。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:safety 内存与并发安全防线                             │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│    线程归属与读写竞态检测   │    分配器一致性校验守卫      │   ASAN 毒化与断言诊断原语     │
│  • thread_id (getCurrent) │  • CheckedAllocator         │  • poison / unpoison         │
│  • ThreadLock (单线程锁)  │  • assertAllocatorEq        │  • isPoisoned / assertPoison │
│  • CriticalSection        │  • assertEq / assertEqFmt   │  • registerRootRegion        │
│    (可重入写 & 并发只读)  │                             │  • safety_assert / panic     │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 四大安全组件详解与代码示例

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
    // 1. 线程 ID 获取与有效性检验
    const tid = getCurrent();
    const ok = isValid(tid) && isCurrent(tid);

    // 2. 单线程归属锁生命周期
    let tlock = ThreadLock.initUnlocked();
    tlock.assertNotLocked();

    tlock.lock();
    tlock.assertLocked();     // 校验当前线程确实持有归属权
    tlock.lockOrAssert();     // 同一线程内幂等通过

    tlock.unlock();
    tlock.assertNotLocked();
}
```

---

### 2. 可重入读写临界区竞态检测器 (`CriticalSection`)

`CriticalSection` 用于保护非线程安全容器免受**多线程并发读写冲突**或**不当的跨线程并发修改**：
- **`begin()` / `end()`（排他写临界区）**：支持同一线程内的嵌套重入（`owned_count += 1`）；若此时存在活跃的只读读者（`shared_readers != 0`）或被其他写线程持有，立即触发竞态诊断 Panic。
- **`beginReadOnly()` / `end()`（共享只读临界区）**：允许多个线程同时进入只读访问；但若当前有任何其他线程处于排他写状态（`is_exclusive == true`），立即拦截报错。

```typescript
import { CriticalSection } from "cap:safety";

pub fn demoCriticalSection() {
    let cs = CriticalSection.init();

    // 1. 同一线程支持嵌套重入排他写
    cs.begin();
    cs.begin(); // 嵌套深度 = 2
    cs.end();   // 嵌套深度 = 1，仍处于锁定态
    cs.end();   // 嵌套深度 = 0，完全释放

    // 2. 共享只读临界区
    cs.beginReadOnly();
    // 执行安全的只读遍历...
    cs.end();
}
```

---

### 3. 非托管容器分配器一致性校验 (`CheckedAllocator`)

对于不保存 `Allocator` 字段的 `Unmanaged` 容器（需要在每次 `append(allocator, ...)` 和 `deinit(allocator)` 时传入分配器），若业务代码不小心在扩容时传入了 `ArenaAllocator`，而在释放时传入了 `page_allocator`，会导致严重的堆损坏。
`CheckedAllocator` 记录首次绑定的 `vtable` 与 `ptr`，在后续每次操作时通过 `set(allocator)` 或 `assertEq(allocator)` 强制校验分配器实例 100% 一致：

```typescript
import std from "std";
import { CheckedAllocator, assertAllocatorEq } from "cap:safety";

pub fn demoCheckedAllocator() {
    const alloc = std.heap.page_allocator;

    // 1. 直接断言两个分配器相同
    assertAllocatorEq(alloc, alloc);

    // 2. 嵌入非托管容器内部的 CheckedAllocator
    let guard = CheckedAllocator.init(alloc);
    guard.set(alloc);      // 校验通过
    guard.assertEq(alloc); // 校验通过

    guard.reset();         // 容器清空释放后可重置绑定
}
```

---

### 4. AddressSanitizer (ASAN) 内存毒化与安全断言 (`asan` & `safety`)

在自定义对象池（`ObjectPool`）或 Arena 内存池中，内存块在归还给池后并未返还给操作系统。通过调用 `poison(ptr, size)` 将空闲槽位标记为毒化状态，一旦外部悬空指针发生 Use-After-Free 读写，ASAN 将立即捕获；当槽位被重新分配时，调用 `unpoison(ptr, size)` 恢复合法访问：

```typescript
import {
    poison,
    unpoison,
    isPoisoned,
    assertUnpoisoned,
    safety_assert,
} from "cap:safety";

pub fn demoAsanAndAssert() {
    let slot_buf: [64]u8 = [0; 64];
    const raw_ptr: *const anyopaque = @ptrCast(&slot_buf);

    // 归还对象池时毒化内存
    poison(raw_ptr, 64);

    // 重新从对象池分配时解除毒化并校验
    unpoison(raw_ptr, 64);
    assertUnpoisoned(raw_ptr);

    safety_assert(!isPoisoned(raw_ptr), "active slot must not be poisoned");
}
```

---

## 三、 与 Bun (`src/safety/`) 对照表

| 安全防线能力 | Bun (`src/safety/*.cp`) | CP 标准库 (`cap:safety`) | 运行时开销 |
| :--- | :--- | :--- | :--- |
| **线程 ID 标识与判别** | `bun.safety.thread_id` | `invalid`, `getCurrent`, `isCurrent`, `isValid` | 寄存器读取 TLS |
| **单线程所有权断言锁** | `bun.safety.ThreadLock` | `ThreadLock` (`lock`, `unlock`, `lockOrAssert`) | 单次 `u64` 比较 |
| **读写竞态检测临界区** | `bun.safety.CriticalSection` | `CriticalSection` (`begin`, `beginReadOnly`, `end`) | 原子计数器校验 |
| **分配器混用防护** | `bun.safety.CheckedAllocator` | `CheckedAllocator`, `assertAllocatorEq` | 2 个指针相等比较 |
| **ASAN 自定义池毒化** | `bun.asan.poison / unpoison` | `poison`, `unpoison`, `isPoisoned`, `registerRootRegion` | 非 ASAN 构建下为 0 |
