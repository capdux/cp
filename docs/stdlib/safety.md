# `cap:safety` — 线程安全、数据竞态防御与 ASAN 诊断标准库

`cap:safety` 模块是 CP 语言官方一等公民标准库中的运行时安全与并发缺陷防御套件，全量重构吸收自 Bun 生产级安全防护体系（`/data/dev/bun/src/safety/`）。本模块提供轻量级线程锁、可重入临界区数据竞态检测、跨分配器防混用检查、当前线程 ID 极速获取以及原生 AddressSanitizer（ASAN）内存毒化集成。

---

## 模块设计理念

1. **零运行时成本的调试与断言支持**：
   - 在 Release 构建或非诊断模式下，检查代码可被编译器激进内联与死代码消除（DCE），达到 0 额外 CPU 与内存损耗；
   - 在 Debug/Test 模式下，全量激活断言、所有权归属判定与并发竞态检测。
2. **多线程并发竞态主动捕捉 (`CriticalSection`)**：
   - 自动检测同一数据被多个线程非同步并发修改或读写冲突；
   - 支持同一线程内的多次可重入嵌套独占（`begin()` ... `end()`），以及跨线程的多读者并发共享（`beginReadOnly()`）。
3. **单线程结构防跨线程误用 (`ThreadLock`)**：
   - 针对假定单线程运行的数据结构（如非线程安全的哈希表、AST 构建器），绑定持有线程 ID，防止被非属主线程并发篡改。
4. **非托管容器跨分配器防混用 (`CheckedAllocator`)**：
   - 追踪非托管集合（Unmanaged Collections）首次绑定的 `Allocator` 虚表与实例指针，当后续方法传入不匹配的分配器时触发确定性防御拦截。
5. **ASAN / LSAN 内存消毒原生整合 (`asan`)**：
   - 为自定义内存池（Arena、Slab、HiveArray）提供显式内存毒化 (`poison`) 与解毒 (`unpoison`)，精准捕捉越界与释放后使用（UAF）。

---

## 核心组件一览

| 组件 | 说明 | 适用场景 |
| :--- | :--- | :--- |
| `thread_id` | 跨平台线程 ID 与无效 ID 常量 (`invalid`, `getCurrent()`, `isCurrent()`, `isValid()`) | 线程亲和性判定、锁持有者身份鉴别 |
| `ThreadLock` | 轻量级线程属主锁与状态断言器 | 确保单线程容器未被多线程无锁并发调用 |
| `CriticalSection` | 原子级可重入独占写 / 多读者共享读竞态检测器 | 调试模式下诊断无锁多线程代码中的数据竞态（Data Race） |
| `CheckedAllocator` | 分配器混用防护包装器 | 防止同一个集合使用 Allocator A 分配却在 Allocator B 中释放 |
| `assertAllocatorEq` | 分配器一致性断言工具 | 容器方法入参校验 |
| `poison` / `unpoison` | ASAN 内存区域手动毒化与解毒 | 自定义堆内存分配器、对象池隔离保护 |
| `describeAddress` | 打印目标地址的 ASAN 分配/释放堆栈追踪 | 内存崩溃与非法访问深度溯源 |
| `registerRootRegion` | 向 LSAN 注册内存根区域以消除伪阳性泄漏 | 自定义无 GC 根集指针注册 |
| `safety_assert` / `safety_panic` | 核心安全断言与不可恢复恐慌原语 | 关键不变式校验 |

---

## API 使用示例

### 1. 线程 ID 提取与有效性校验 (`thread_id`)

```typescript
import { getCurrent, isCurrent, isValid, invalid } from "cap:safety";

const tid = getCurrent();
assert(isValid(tid) == true);
assert(isCurrent(tid) == true);
assert(isValid(invalid) == false);
```

---

### 2. 线程单归属锁 (`ThreadLock`)

用于防止单线程实例被并发篡改：

```typescript
import { ThreadLock } from "cap:safety";

let lock = ThreadLock.initUnlocked();

// 1. 获取锁并确立属主
lock.lock();
assert(lock.isLocked() == true);
assert(lock.isHeldByCurrentThread() == true);
lock.assertLocked(); // 确保当前线程持有

// 2. 释放锁
lock.unlock();
assert(lock.isLocked() == false);
lock.assertNotLocked();

// 3. lockOrAssert：如果未锁则加锁，如果已由当前线程持有则通过断言
lock.lockOrAssert();
lock.lockOrAssert(); // 同线程幂等通过
lock.unlock();
```

---

### 3. 可重入临界区并发竞态检测 (`CriticalSection`)

可在数据结构外围或内部部署，检测并发读写冲突：

```typescript
import { CriticalSection } from "cap:safety";

let cs = CriticalSection.init();

// --- 场景 A: 独占写锁与同一线程可重入嵌套 ---
cs.begin(); // 首次进入独占临界区
cs.begin(); // 同一线程允许重入嵌套

// 执行写操作...

cs.end();   // 递减重入层级
cs.end();   // 完全退出临界区，释放属主
assert(cs.isLocked() == false);

// --- 场景 B: 多读者共享读锁 ---
cs.beginReadOnly(); // 标记共享读
// 允许其他线程同时调用 beginReadOnly() 共同阅读...
cs.end();
```

> [!NOTE]
> 当某个线程正在持有独占写锁时，任何其他线程尝试执行 `begin()` 或 `beginReadOnly()` 均会被立即捕获并输出详细的竞态线程诊断。

---

### 4. 容器分配器一致性校验 (`CheckedAllocator`)

针对非托管（Unmanaged）数据结构，防止跨生命周期分配器混用：

```typescript
import { CheckedAllocator, assertAllocatorEq } from "cap:safety";
import std from "std";

const page_allocator = std.heap.page_allocator;

// 1. 直接断言两分配器同源
assertAllocatorEq(page_allocator, page_allocator);

// 2. 容器生命周期绑定
let checked = CheckedAllocator.init(page_allocator);
assert(checked.isSet() == true);

// 传入匹配的分配器校验通过
checked.set(page_allocator);
checked.assertEq(page_allocator);

// 重置并解绑
checked.reset();
assert(checked.isSet() == false);
```

---

### 5. AddressSanitizer 内存标记集成 (`asan`)

用于开发自定义内存分配器或无 GC 缓存池时，主动告知编译器/运行时目标内存的有效状态：

```typescript
import {
    poison,
    unpoison,
    isPoisoned,
    assertPoisoned,
    assertUnpoisoned,
    describeAddress,
} from "cap:safety";

let dummy: u64 = 0xDEADBEEF;
const ptr: *const anyopaque = @ptrCast(&dummy);

// 将内存标记为不可读写（毒化）
poison(ptr, 8);

// 恢复内存正常访问权限（解毒）
unpoison(ptr, 8);
```

---

## 质量与测试验证

`cap:safety` 经受了 `cap/test/test_safety_complete.cp` 全套测试套件的严苛检验：

- **Test 1**: `thread_id` 平台原生线程获取、哨兵值与有效性校验；
- **Test 2**: `ThreadLock` 生命周期、加锁属主判定与断言检测；
- **Test 3**: `CriticalSection` 可重入嵌套写、多读者共享读以及序列化切换；
- **Test 4**: `CheckedAllocator` 容器分配器绑定、重置与同源断言；
- **Test 5**: `ASAN` 接口桩调用、地址描述与安全断言。

全套测试用例通过 `cap test cap/test/test_safety_complete.cp` 达成 **100% PASS (685ms)**。
