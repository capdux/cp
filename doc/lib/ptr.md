# CP 标准库：智能指针、引用计数与写时复制内存抽象使用手册 (`cap:ptr`)

## 一、 架构总览与设计哲学

`cap:ptr` 是 CP 语言官方内置的**零 GC 高性能智能指针、侵入式/非侵入式引用计数、写时复制（Copy-on-Write）与 64 位打标指针（Tagged Pointer）**标准库。完整吸收自 Bun（`/data/dev/bun/src/ptr/`），为 CP 引擎在无需追踪式垃圾回收（Tracing GC）的前提下，提供媲美 Rust `Arc`/`Rc`/`Cow`/`Box` 与 C++ `std::shared_ptr`/`boost::intrusive_ptr` 的确定性内存生命周期管理。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:ptr 智能指针与所有权矩阵                              │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│    强/弱引用计数体系       │   写时复制与独占所有权容器   │     指针位压缩与 FFI 桥接     │
│  • Shared<*T> & Weak<*T>  │  • Cow<T> (值写时复制)      │  • TaggedPointer (高16位Tag) │
│  • AtomicShared / Weak    │  • CowSlice<T> / CowString  │  • LowTagPointer (低3位Tag)  │
│  • RefCount / ThreadSafe  │    (u63长度 + 1位所有权位)  │  • ExternalShared<T> (C-FFI) │
│  • RefPtr<T> & WeakPtr<T> │  • Owned<*T> / Owned<[]T>   │  • PointerInfo / AddConst    │
│  • RawRefCount<Int,Mode>  │    (唯一独占所有权 RAII)    │                              │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 核心机制深度解析

### 1. 64 位高 16 位与低 3 位指针打标压缩 (`TaggedPointer` & `LowTagPointer`)

在现代 64 位架构（x86_64 48-bit 虚拟地址与 ARM64 TBI）下，用户态虚拟地址仅占用低 48 位（`0x0000FFFFFFFFFFFF`）：
- **`TaggedPointer`（高 16 位打标）**：将高 16 位（Bits `48..63`）用于存储类型标签、状态机阶段或枚举判别码（`u16`），低 48 位保存物理指针地址，在不增加任何额外字节（保持 8 字节单寄存器宽度）的情况下实现零虚表（Zero-VTable）多态联合体。
- **`LowTagPointer`（低 3 位对齐打标）**：利用 8 字节对齐指针低 3 位恒为 `0` 的硬件特性（`LOW_TAG_MASK = 0x7`），在低 3 位嵌入 `0..7` 的紧凑标志位。

### 2. 16 字节指针打包写时复制切片 (`CowSlice<T>` & `CowString`)

标准切片 `[ptr: 8B, len: 8B]` 占用 16 字节。若额外增加一个 `is_owned: bool` 字段，由于 8 字节内存对齐，结构体将膨胀至 24 字节。
`CowSlice<T>` 采用 `@packed(usize) struct CowSliceFlags { len: u63, is_owned: bool }` 将 `is_owned` 所有权标志位直接打包进 `usize` 长度字段的最高位（Bit 63）：
- 静态字面量（`initStatic`）或借用子切片（`borrowSubslice`）时，`is_owned = false`，`deinit()` 为零开销空操作；
- 仅在调用 `sliceMut(allocator)` 或 `toOwned(allocator)` 请求可变修改时，才触发一次堆内存拷贝并将 `is_owned` 置为 `true`。

### 3. 非侵入式共享指针 vs 侵入式引用计数 (`Shared<*T>` vs `RefCount` + `RefPtr<T>`)

- **`Shared<*T>` / `AtomicShared<*T>`**：通过 `FullData` 将控制块（`strong_count`、`weak_count`、`allocator`、`thread_lock`）与载荷 `value: T` 在单次堆分配中连续布局，通过 `@fieldParentPtr("value", ptr)` 实现裸指针与控制块之间的 $O(1)$ 零偏移换算，并支持 `Weak<*T>.upgrade()` 安全防悬空升级。
- **`RefCount` / `ThreadSafeRefCount` + `RefPtr<T>`**：侵入式混入目标结构体字段（如 `ref_count`），零外部控制块开销，单线程模式下内置 `ThreadLock` 自动拦截跨线程误用。

---

## 三、 完整可运行代码示例

### 1. 强弱引用计数生命周期 (`Shared` / `Weak` / `AtomicShared`)

```typescript
import std from "std";
import { Shared, AtomicShared } from "cap:ptr";

type PtrI32 = *i32;
type PtrU64 = *u64;

pub fn demoSharedAndWeak() {
    // 1. 单线程带 ThreadLock 保护的强/弱引用指针
    let s1 = Shared(PtrI32).new(42);
    let s2 = s1.clone();
    s2.get().* = 100; // s1.get().* 同步变为 100

    // 创建弱引用并尝试升级为强引用
    let weak = s1.cloneWeak();
    if (upgraded := weak.upgrade()) {
        let up = upgraded;
        // up.strongCount() == 3
        up.deinit();
    }

    s1.deinit();
    s2.deinit();
    // 此时所有强引用已释放，weak.upgrade() 安全返回 null，绝无 Use-After-Free
    const expired = weak.upgrade(); // null
    weak.deinit();

    // 2. 跨线程原子引用计数指针
    let a1 = AtomicShared(PtrU64).new(2026);
    let a2 = a1.clone();
    a1.deinit();
    a2.deinit();
}
```

---

### 2. 写时复制值与切片 (`Cow<T>` & `CowSlice<u8>`)

```typescript
import std from "std";
import { Cow, CowSlice } from "cap:ptr";

pub fn demoCow() {
    const allocator = std.heap.page_allocator;

    // 1. Cow<T> 值包装器
    let origin: i32 = 10;
    let cow_val = Cow(i32).borrow(&origin);
    // 借用状态下只读访问零拷贝
    const ptr = cow_val.toOwned(allocator); // 首次写操作触发独立拷贝
    ptr.* = 99; // origin 保持 10 不变
    cow_val.deinit(allocator);

    // 2. CowSlice<u8> (CowString) 零拷贝子切片借用与写时克隆
    let str_cow = CowSlice(u8).initStatic("antigravity_engine");
    let sub = str_cow.borrowSubslice(0, 11); // "antigravity" (is_owned == false)

    let mut_buf = sub.sliceMut(allocator) catch unreachable;
    mut_buf[0] = 'A'; // sub.slice() 变为 "Antigravity"，原静态切片不受影响
    sub.deinit(allocator);
}
```

---

### 3. 独占所有权与 64 位打标指针 (`Owned` / `TaggedPointer` / `LowTagPointer`)

```typescript
import { Owned, TaggedPointer, LowTagPointer } from "cap:ptr";

type PtrI64 = *i64;

pub fn demoOwnedAndTagged() {
    // 1. 唯一独占所有权指针 Owned<*T>
    let box_val = Owned(PtrI64).new(987654321);
    box_val.get().* += 1;
    box_val.deinit();

    // 2. 高 16 位打标指针 TaggedPointer
    let payload: u64 = 0xCAFEBABE;
    let tp = TaggedPointer.fromPtr(&payload, 0x1234);
    const tag16 = tp.tag();           // 0x1234
    const deref_val = tp.get(u64).*;  // 0xCAFEBABE

    // 3. 低 3 位对齐打标指针 LowTagPointer
    let lp = LowTagPointer.fromPtr(&payload, 0x5);
    const tag3 = lp.tag();            // 5
}
```

---

## 四、 与 Bun (`src/ptr/`) / Rust 标准库对照表

| 内存原语 | Rust 标准库 | Bun (`src/ptr/*.cp`) | CP 标准库 (`cap:ptr`) |
| :--- | :--- | :--- | :--- |
| **唯一独占堆指针** | `Box<T>` / `Box<[T]>` | `bun.ptr.Owned(T)` | `Owned(*T)` / `Owned([]T)` |
| **单线程强/弱共享指针**| `Rc<T>` / `Weak<T>` | `bun.ptr.Shared(*T)` | `Shared(*T)` / `Weak(*T)` |
| **多线程原子共享指针** | `Arc<T>` / `sync::Weak<T>` | `bun.ptr.AtomicShared(*T)` | `AtomicShared(*T)` / `AtomicWeak(*T)` |
| **侵入式引用计数** | 需第三方 `intrusive_collections`| `bun.ptr.RefCount` / `RefPtr`| `RefCount` / `ThreadSafeRefCount` / `RefPtr` |
| **写时复制容器/切片** | `Cow<'a, T>` / `Cow<'a, str>` | `Cow(T)` / `CowSlice(T)` | `Cow(T)` / `CowSlice(T)` / `CowString` |
| **高16位/低3位打标指针**| 需第三方 `tagged_ptr` | `TaggedPointer` | `TaggedPointer` / `LowTagPointer` |
