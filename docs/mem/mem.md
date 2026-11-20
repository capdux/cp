# CP 标准库：区域内存分配器、配额管理与引用计数子系统使用手册 (`cap:mem`)

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
