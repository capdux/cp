# `cap:ptr` — 智能指针与底层内存抽象标准库

`cap:ptr` 模块提供一套高性能、高安全性、零 GC 依赖的工业级智能指针与内存语义抽象，完整兼容并发 Actor 模型与底层硬件指针操作。

## 模块设计理念

1. **确定性生命周期与显式所有权**：在无通用 GC 的 Native 运行环境下，通过借用检查、引用计数与 RAII 语义保证内存 100% 确定性析构；
2. **多线程安全配置**：支持单线程极致零开销（非原子计数）与跨线程无锁并发（原子计数与内存屏障）双模式无缝切换；
3. **Copy-On-Write 写时复制优化**：针对只读复用与突发修改场景，提供标量与切片级的零拷贝共享与自动克隆升级；
4. **底层硬件指针位压缩**：针对 64 位机器高低位未用虚拟地址空间，提供基于硬件指针压缩的高性能 `TaggedPointer` 与 `LowTagPointer`。

---

## 核心组件一览

| 组件 | 说明 | 适用场景 |
| :--- | :--- | :--- |
| `Shared<T>` | 单线程引用计数共享指针 | 单线程或单 Actor 内部复杂图结构、多重所有权共享 |
| `AtomicShared<T>` | 线程安全原子引用计数共享指针 | 跨线程、跨 Actor 零拷贝安全共享大数据与上下文 |
| `Weak<T>` / `AtomicWeak<T>` | 弱引用指针，解决循环引用 | 缓存层、父子节点反向指针、观察者模式 |
| `Cow<T>` | 写时复制标量包装器 (Borrowed / Owned) | 静态/动态配置合并、默认常量值覆写 |
| `CowSlice<T>` / `CowString` | 写时复制切片与字符串 | 文本解析、分词、零拷贝缓冲区切片 |
| `Owned<T>` / `Dynamic<T>` | 独占所有权智能指针 | 独占堆资源、单写者管道、零拷贝所有权转移 (`move`) |
| `RefCount<T>` / `RefPtr<T>` | 侵入式引用计数机制 | 高频内部对象管理、无双重间接指针开销 |
| `TaggedPointer<T>` | 64 位高位标记压缩指针 (48-bit 地址 + 16-bit 标签) | AST 节点压缩、虚拟机多态值编码、内存紧凑缓存 |
| `LowTagPointer<T>` | 64 位低位对齐标记指针 (基于对齐空出的低 2~3 位) | 高频运行时指针标记、快速判空与类型甄别 |

---

## API 使用示例

### 1. `Shared<T>` 与 `Weak<T>`

```typescript
import { Shared, Weak } from "cap:ptr";

type PtrI32 = *i32;

// 创建单线程共享指针
let s1 = Shared(PtrI32).new(42);
assert(s1.get().* == 42);
assert(s1.strongCount() == 1);
assert(s1.isUnique() == true);

// 克隆增加强引用计数
let s2 = s1.clone();
assert(s1.strongCount() == 2);
assert(s1.isUnique() == false);

// 降级为弱引用，不增加强引用计数
let w1 = s1.downgrade();
assert(w1.strongCount() == 2);

// 强引用指针手动或离开作用域释放
s2.deinit();
assert(s1.strongCount() == 1);

// 通过弱引用升级为有效强引用
let upgraded = w1.upgrade();
if (upgraded != null) {
    let strong = upgraded.?;
    assert(strong.get().* == 42);
    strong.deinit();
}
s1.deinit();
```

---

### 2. `AtomicShared<T>` 跨线程原子共享

```typescript
import { AtomicShared } from "cap:ptr";

type PtrU64 = *u64;

// 线程安全原子计数指针
let shared_data = AtomicShared(PtrU64).new(10086);

// 跨线程/跨 Actor 安全传递克隆引用
let worker_ref = shared_data.clone();
assert(shared_data.strongCount() == 2);

worker_ref.deinit();
assert(shared_data.strongCount() == 1);
shared_data.deinit();
```

---

### 3. `CowString` 与 `CowSlice<T>` 写时复制

```typescript
import { CowString } from "cap:ptr";
import std from "std";

// 从静态不可变字面量借用（零内存分配）
let cow = CowString.initBorrowed("Hello World");
assert(cow.isBorrowed() == true);
assert(cow.slice() == "Hello World");

// 读取无需复制；若需变更，调用 toOwned 或 ensureOwned 触发自动克隆
let owned_slice = cow.toOwned(std.heap.page_allocator)?;
assert(cow.isOwned() == true);

cow.deinit(std.heap.page_allocator);
```

---

### 4. `Owned<T>` 独占所有权指针

```typescript
import { Owned } from "cap:ptr";
import std from "std";

type PtrI64 = *i64;
type SliceU8 = []u8;

// 单标量独占分配
let single = Owned(PtrI64).new(987654321);
assert(single.get().* == 987654321);
single.get().* = 123456789;
assert(single.get().* == 123456789);
single.deinit();

// 连续切片独占分配
let slice = Owned(SliceU8).allocIn(16, 0xAA, std.heap.page_allocator)?;
assert(slice.get().len == 16);
assert(slice.get()[0] == 0xAA);
slice.deinit();
```

---

### 5. `TaggedPointer` 硬件地址位压缩

```typescript
import { TaggedPointer } from "cap:ptr";

let target_value: u64 = 0xDEADBEEF;
let tag_id: u16 = 0x55AA;

// 将 48 位指针与 16 位标签压缩存储为单 64 位整数
let tp = TaggedPointer.fromPtr(&target_value, tag_id);
assert(tp.getTag() == 0x55AA);
assert(tp.getPtr().* == 0xDEADBEEF);

// 原地修改标签而不影响原始物理指针
tp.setTag(0x1234);
assert(tp.getTag() == 0x1234);
assert(tp.getPtr().* == 0xDEADBEEF);
```

---

## 溯源关系
本模块组件完全对标并吸收自 Bun 生产环境指针与内存基础设施：
- `lib/ptr/meta.cp`: `// Source: bun/src/ptr/meta.cp`
- `lib/ptr/raw_ref_count.cp`: `// Source: bun/src/ptr/raw_ref_count.cp`
- `lib/ptr/ref_count.cp`: `// Source: bun/src/ptr/ref_count.cp`
- `lib/ptr/shared.cp`: `// Source: bun/src/ptr/shared.cp`
- `lib/ptr/weak_ptr.cp`: `// Source: bun/src/ptr/weak_ptr.cp`
- `lib/ptr/cow.cp`: `// Source: bun/src/ptr/Cow.cp`
- `lib/ptr/cow_slice.cp`: `// Source: bun/src/ptr/cow_slice.cp`
- `lib/ptr/owned.cp`: `// Source: bun/src/ptr/owned.cp`
- `lib/ptr/tagged_pointer.cp`: `// Source: bun/src/ptr/tagged_pointer.cp`
- `lib/ptr/external_shared.cp`: `// Source: bun/src/ptr/external_shared.cp`
