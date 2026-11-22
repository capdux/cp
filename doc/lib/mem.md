# CP 标准库：区域内存分配器、配额管理与引用计数子系统使用手册 (`cap:mem`)

> 参考来源：`/data/dev/bun/src/bun_alloc/bun_alloc.cp`, `/data/dev/bun/src/bun_alloc/MaxHeapAllocator.cp`, `/data/dev/bun/src/ptr/` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:mem` 是 CP 语言官方内置的**高性能内存分配器（`MaxHeapAllocator`、`StackFallbackAllocator`、`BumpArena`、`Arena`）、Mimalloc C-FFI 底层绑定、跨 Actor 零拷贝共享张量（`SharedTensor`）、所有权感知缓冲区（`MaybeOwnedBuf`、`CowBytes`、`CowSlice`、`Cow`）、引用计数器（`RefCounter`、`AtomicRefCounter`、`WeakControlBlock`、`Rc`）与 64 位打标指针（`TaggedPointer`、`LowTagPointer`、`TaggedPtr`）**标准库。完整吸收自 Bun（`/data/dev/bun/src/bun_alloc/` 与 `/data/dev/bun/src/ptr/`）。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:mem 内存分配与零拷贝体系                              │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   高性能专用内存分配器     │   零拷贝缓冲区与写时复制     │   引用计数、打标与 Mimalloc   │
│  • MaxHeapAllocator       │  • SharedTensor (原子引用)  │  • RefCounter / Rc           │
│    (高水位复用与配额限制) │  • MaybeOwnedBuf            │  • AtomicRefCounter          │
│  • StackFallbackAllocator │  • CowBytes / CowSlice / Cow│  • WeakControlBlock          │
│  • BumpArena (线性对齐)   │                             │  • TaggedPointer / TaggedPtr │
│  • Arena (@alloc/@free)   │                             │  • MiHeap / mi_malloc (FFI)  │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 核心分配器与内存原语详解

### 1. 高水位单次复用与配额限制分配器 (`MaxHeapAllocator`)

源自 `/data/dev/bun/src/bun_alloc/MaxHeapAllocator.cp`。在解析器、压缩器或循环批处理任务中，往往需要反复申请临时工作缓冲区：
- **高水位线（High-Water-Mark）零碎片复用**：当新请求的 `len <= capacity()` 时，直接复用已分配的同一块底层内存，零系统调用；仅当 `len > capacity()` 时才扩容至新高水位；
- **硬性堆上限配额保护（`max_limit`）**：若初始化时指定了非零 `max_limit`（如 `MaxHeapAllocator.init(64 * 1024)`），任何超过配额上限的分配请求将安全返回 `null`，防止异常报文耗尽系统内存。

```typescript
import { MaxHeapAllocator } from "cap:mem";

pub fn demoMaxHeapAllocator() {
    // 创建最大配额上限为 1024 字节的高水位复用分配器
    let mha = MaxHeapAllocator.init(1024);
    defer mha.release();

    // 首次申请 256 字节 -> 触发底层分配 (capacity == 256)
    const buf1 = mha.alloc(256).?;

    // 再次申请 128 字节 -> 100% 零开销复用原缓冲区 (capacity 保持 256, used == 128)
    const buf2 = mha.alloc(128).?;

    // 申请 2048 字节 -> 超过 maxQuota (1024)，安全拦截并返回 null
    const over_quota = mha.alloc(2048); // null
}
```

---

### 2. 栈优先回退分配器与线性对齐区域分配器 (`StackFallbackAllocator<CAP>` & `BumpArena`)

源自 `/data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp`：
- **`StackFallbackAllocator<CAP>`**：优先从栈内定长数组 `[CAP]u8` 中以指针碰撞（Bump Pointer）方式极速切分内存；当栈剩余空间不足时返回 `null` 并通过 `recordFallback(len)` 统计回退到堆的次数与字节数；
- **`BumpArena`**：连续大块内存线性分配器，支持按字节分配 `allocBytes(len)` 与按 $2^N$ 对齐分配 `allocAligned(len, align_pow2)`，调用 `reset()` 即可在 $O(1)$ 时间内将游标归零循环复用。

```typescript
import { StackFallbackAllocator, BumpArena } from "cap:mem";

pub fn demoStackAndBumpArena() {
    // 1. 栈优先分配器 (64 字节栈缓冲)
    let sfa = StackFallbackAllocator<64>.init();
    if (stack_slice := sfa.allocStack(32)) {
        stack_slice[0] = 0xFF;
    }
    sfa.reset();

    // 2. 线性对齐区域分配器 BumpArena
    let ba = BumpArena.initCapacity(4096);
    defer ba.release();

    const raw = ba.allocBytes(13).?;
    const simd_aligned = ba.allocAligned(64, 16).?; // 16 字节对齐切片
    ba.reset(); // O(1) 重置偏移量
}
```

---

### 3. 所有权感知缓冲、写时复制与跨 Actor 共享张量 (`MaybeOwnedBuf` / `Cow` / `SharedTensor`)

- **`MaybeOwnedBuf`**：轻量级包装 `[]const u8` 与 `owned: bool`，无论是借用静态切片（`initBorrowed`）还是堆拷贝副本（`initOwnedCopy`），统一调用 `release()` 即可安全处理生命周期；
- **`CowBytes` / `CowSlice` / `Cow`**：写时复制字节容器，`borrow()` 零拷贝共享底层指针，调用 `setByte(idx, val)` 或 `ensureOwned()` 时自动在堆上克隆独立副本；
- **`SharedTensor`**：专为 AI Agent 大体积张量通信设计的原子引用计数（`retain()` / `release()`）零拷贝共享内存块。

```typescript
import {
    MaybeOwnedBuf,
    CowBytes,
    Cow,
    SharedTensor,
} from "cap:mem";

pub fn demoBuffersAndTensor() {
    // 1. Cow 写时复制字节流
    let c1 = Cow.initBorrowed("hello");
    let c2 = CowBytes.initBorrowed(c1.slice());
    c2.setByte(0, 'H'); // 触发写时复制：c2 变为 "Hello"，c1 仍为 "hello"
    c2.release();
    c1.release();

    // 2. SharedTensor 跨 Actor 原子引用计数内存块
    const tensor = new SharedTensor(4096);
    tensor.retain();  // ref_count == 2
    tensor.release(); // ref_count == 1
}
```

---

### 4. 引用计数控制块与 64 位打标指针 (`Rc` / `RefCounter` / `AtomicRefCounter` / `WeakControlBlock` / `TaggedPtr`)

```typescript
import {
    Rc,
    AtomicRefCounter,
    WeakControlBlock,
    DecrementResult,
    TaggedPointer,
    LowTagPointer,
    TaggedPtr,
} from "cap:mem";

pub fn demoRcAndTaggedPtr() {
    // 1. 单线程与原子引用计数器
    let rc = Rc.init(1);
    rc.increment(); // 2
    const res1 = rc.decrement(); // DecrementResult.KeepAlive
    const res2 = rc.decrement(); // DecrementResult.ShouldDestroy

    // 2. 弱引用控制块状态机
    let wcb = WeakControlBlock.init();
    wcb.incWeak();
    wcb.decStrong(); // finalized = true, 但因 weak_count == 1 尚不可释放控制块
    const can_free = wcb.decWeak(); // true (strong == 0 && weak == 0)

    // 3. 高 16 位与低 3 位打标指针
    const tp = TaggedPtr.pack(0x00007FFF12345678, 0xABCD);
    const addr = tp.address(); // 0x00007FFF12345678
    const tag = tp.tag();      // 0xABCD
}
```

---

## 三、 与 Bun (`src/bun_alloc/`) 对照表

| 内存管理组件 | Bun (`src/bun_alloc/*.cp`) | CP 标准库 (`cap:mem`) | 核心工程优势 |
| :--- | :--- | :--- | :--- |
| **高水位单次复用分配器**| `MaxHeapAllocator` | `MaxHeapAllocator` | 高水位零碎片复用 + `max_limit` 硬配额防护 |
| **栈优先回退分配器** | `BufferFallbackAllocator` | `StackFallbackAllocator<CAP>` | 热路径小对象 100% 栈分配 |
| **连续线性区域分配器** | `MimallocArena` / `Arena` | `BumpArena` / `Arena` | 支持 $2^N$ 对齐分配与 $O(1)$ `reset` |
| **所有权感知只读缓冲** | `MaybeOwned` | `MaybeOwnedBuf` | 统一借用与拥有的释放契约 |
| **写时复制字节切片** | `CowSlice` | `CowBytes`, `CowSlice`, `Cow` | 惰性堆克隆，读取零开销 |
| **Mimalloc C-FFI 绑定**| `bun.mimalloc` | `MiHeap`, `mi_malloc`, `mi_heap_*`| 支持线程独立堆（`MiHeap`）与 0 碎片分配 |
| **固定缓冲分配器** | `FixedBufferAllocator` | `FixedBufferAllocator` (`alloc`, `allocAligned`, `free`, `reset`) | 零堆分配，支持对齐分配与 LIFO 回退 |
| **定长对象池** | `SlabPool` | `SlabPool` (`alloc`, `free`, `contains`, `reset`) | $O(1)$ 空闲链分配回收 + 防 double-free 校验 |
| **对齐与安全清零原语** | `memory.cp` | `isPowerOfTwo`, `alignForward`, `alignBackward`, `alignDelta`, `secureZero` | 2 的幂对齐原语 + `volatile` 防编译器优化敏感清零 |

---

### 5. 固定缓冲分配器 (`FixedBufferAllocator`)、$O(1)$ 对象池 (`SlabPool`) 与内存原语

- **`FixedBufferAllocator`**：基于调用方传入切片 `[]u8` 进行零堆分配管理，支持 `alloc(len)`、`allocAligned(len, alignment)`、LIFO 尾部释放 `free(slice)`、`ownsSlice`、`used()`、`remaining()` 与 `reset()`。
- **`SlabPool`**：定长槽位空闲链表池，提供 $O(1)$ `alloc()` 与 `free(slice)`，内置边界/步长对齐校验与 `in_use` 状态位图，在遭遇 double-free 或外部非法切片时安全返回 `false` 防止链表破坏。
- **对齐与安全擦除原语**：`isPowerOfTwo(n)`、`alignForward(addr, align)`、`alignBackward(addr, align)`、`alignDelta(addr, align)`，以及通过 `volatile` 指针写保证不被死存储消除（DSE）的 `secureZero(buf)`。

```typescript
import {
    isPowerOfTwo, alignForward, alignBackward, secureZero,
    FixedBufferAllocator, SlabPool,
} from "cap:mem";

pub fn demoFixedAndSlabPool() {
    // 1. 对齐与安全清零原语
    const aligned = alignForward(13, 8); // 16
    let secret: [16]u8 = [0xAA; 16];
    secureZero(secret[0..16]);

    // 2. FixedBufferAllocator (零堆分配)
    let backing: [256]u8 = [0; 256];
    let fba = FixedBufferAllocator.init(backing[0..256]);
    const simd_buf = fba.allocAligned(32, 16).?;
    fba.free(simd_buf);
    fba.reset();

    // 3. SlabPool (O(1) 空闲链 + 防 double-free)
    let pool = SlabPool.init(64, 8);
    defer pool.release();
    const slot = pool.alloc().?;
    const ok1 = pool.free(slot); // true
    const ok2 = pool.free(slot); // false (安全拦截 double-free)
}
```

---

### 6. 泄漏追踪包装分配器 (`AllocationScope`)、泛型回退 (`BufferFallbackAllocator<N>` / `MaybeOwned<T>`) 与 `MemFdBuffer`

新增源自 `/data/dev/bun/src/bun_alloc/` 的三大生产级组件：

- **`AllocationScope`** (`lib/mem/allocationScope.cp`)：
  - 完整包装 `std.mem.Allocator` 虚表（`scope.allocator()`），同时提供直调方法 `allocBytes` / `freeBytes` / `trackExternalAlloc` / `trackExternalFree`；
  - 实时统计 `liveAllocations()`、`liveBytes()`、`totalAllocatedBytes()`、`peakBytes()`、`totalFrees()`；
  - 精准拦截重复释放（`ScopeError.DoubleFree` + `doubleFrees()`）与悬空/未登记指针释放（`ScopeError.InvalidFree` + `invalidFrees()`）；
  - 提供 `assertNoLeaks()`、`isOwned()`、`formatReport()` / `formatReportAlloc()`。
- **`BufferFallbackAllocator<const N: usize>`** (`lib/mem/fallbackAllocator.cp`)：
  - 内置 `N` 字节栈定长缓冲，热路径优先栈上 bump 分配（`allocStack` / `allocBytes` / `allocAligned` / `allocator()`），溢出自动无缝回退至堆分配器并记录 `heapAllocs()` / `heapBytes()`。
- **`MaybeOwned<T>`** (`lib/mem/fallbackAllocator.cp`)：
  - 泛型零成本统一借用切片（`initBorrowed`）与堆拥有切片（`initOwned` / `initOwnedCopy`），支持 `asSlice()`、`toOwned()`（原地按需克隆提升为拥有态）与 `deinit()`。
- **`MemFdBuffer`** (`lib/mem/fallbackAllocator.cp`)：
  - 基于 Linux `memfd_create` + `ftruncate` + `pwrite` + `mmap(MAP.SHARED)` 的匿名内存文件缓冲区，支持 `asSlice()` / `asMutSlice()`、`readFd()` / `writeFd()`、`cloneView()` 零拷贝共享视图映射与 `fd()` 文件描述符传递。

```typescript
import {
    AllocationScope,
    BufferFallbackAllocator,
    MaybeOwned,
    MemFdBuffer,
} from "cap:mem";

pub fn demoPhaseE1Mem(): ! {
    // 1. AllocationScope 泄漏与峰值追踪
    let scope = AllocationScope.initDefault()?;
    defer scope.deinit();
    const b = scope.allocBytes(256, "packet")?;
    // scope.liveAllocations() == 1, scope.peakBytes() == 256
    scope.freeBytes(b)?;
    scope.assertNoLeaks()?;

    // 2. BufferFallbackAllocator<64> 栈热路径 + 堆回退
    let fba = BufferFallbackAllocator<64>.initDefault();
    const s1 = fba.allocBytes(32)?; // 栈上分配 (fba.ownsSlice(s1) == true)
    const s2 = fba.allocBytes(64)?; // 自动回退堆分配 (fba.heapAllocs() == 1)
    fba.freeBytes(s2);
    fba.freeBytes(s1);

    // 3. MaybeOwned<u8> & MemFdBuffer
    let mo = MaybeOwned<u8>.initBorrowed("hello");
    defer mo.deinit();
    const owned = mo.toOwned()?;
    owned[0] = 'H';

    let mfd = MemFdBuffer.create("zero-copy-memfd", "demo")?;
    defer mfd.deinit();
}
```

---

## 上游 Bun 内存分配器行为契约与安全边界

1. **`MaxHeapAllocator` 高水位复用与配额拦截**：
   - 当请求大小 `<= capacity()` 时直接复用已有块（零系统调用）；超过 `max_limit` 时返回 `null` 而不崩溃，适合解析外部未知长度报文的硬上限防护。
2. **`StackFallbackAllocator` 栈热路径与堆回退**：
   - 小对象优先从固定栈缓冲线性分配，超容量后无缝回退至后备堆分配器；注意栈内联切片的生命周期严格绑定分配器实例所在栈帧，不得逃逸返回至外层作用域。
3. **`TaggedPointer` / `LowTagPointer` 对齐与地址位宽约束**：
   - 低位打标（`LowTagPointer`）依赖目标指针自然对齐产生的低位全零位；高位打标（`TaggedPointer`）利用 64 位虚拟地址高 16 位空闲位，解引用前必须先还原掩码（`ADDR_MASK`）。
4. **Mimalloc 线程本地堆隔离（`MiHeap`）**：
   - `mi_heap_new()` 创建的专属堆支持 `mi_heap_destroy()` 一次性 O(1) 整体回收，适合请求级或任务级短暂生命周期批处理。
5. **紧凑 8 字节外部偏移视图（`ExternalSlice<T>` / `ExternalSliceMap<T>` / `ExternalBytePool`，`externalSlice.cp`）**：
   - 吸收自 `/data/dev/bun/src/install/ExternalSlice.cp`（参考 `/data/dev/bun/docs/runtime/binary-data.mdx` 中零拷贝二进制视图实践）：用 8 字节 `(off: u32, len: u32)` 取代 16 字节指针切片，内存占用减半且不受底层 `ArrayBuffer`/Arena 扩容重分配影响；
   - `get(buf)` / `mut(buf)` 提供安全钳位零拷贝视图，`getChecked(buf)` / `mutChecked(buf)` 与 `subslice(rel_off, rel_len)` 提供严格越界与 `u32` 溢出防护；
   - 配合 `ExternalBytePool`（调用方提供 `[]u8` 缓冲区）与 `ExternalSliceMap<T>` 可零堆分配构建二进制协议表、Lockfile 字符串池与紧凑 AST 索引。

```cp
import { ExternalBytePool, ExternalSlice } from "cap:mem";

let backing: [64]u8 = [0; 64];
let pool = ExternalBytePool.init(backing[0..]);
if (h1 := pool.append("bun-lockfile")) {
    const view = pool.get(h1); // "bun-lockfile" (zero-copy)
    * = view;
}
const inv = ExternalSlice<u8>.invalid();
* = inv.isInvalid(); // true
```
