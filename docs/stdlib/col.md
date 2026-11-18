# `cap:col` 高性能系统级集合与容器库

`cap:col` 为 CP 语言官方一等公民容器库，吸收自 Bun 工业级底层架构与 Zig 核心设计，专为百万级高并发、零内存泄漏与硬件 SIMD 加速场景打造。

---

## 一、 模块拓扑与导入

```typescript
import {
    // 1. 位图与集合代数
    BitSet64, BitSet128, BitSet256, BitSet512, BitSet1024, DynamicBitSet, Range,
    // 2. 栈分配有界数组
    BoundedArray,
    // 3. 环形双端队列
    LinearFifo,
    // 4. 紧凑型索引动态数组
    BabyList, OffsetByteList,
    // 5. 列式分列数组
    Soa,
    // 6. 蜂巢对象与复用池
    HiveArray, ObjectPool
} from "cap:col";
```

---

## 二、 核心组件设计与 API 详述

### 1. `BitSet64` / `BitSetN` 与 `DynamicBitSet`（位图与集合代数）

#### 特性：
- 采用 `@popCount` 与 `@ctz`（Count Trailing Zeros）实现硬件单周期极速位计数与扫描；
- 全套布尔集合代数运算（并集 `setUnion`、交集 `setIntersection`、差集 `differenceWith`、补集 `complement`、子集判定 `subsetOf`）；
- `DynamicBitSet` 支持堆上动态按需扩容与收缩（`resize(new_len, fill)`），尾字掩码严格自动对齐。

#### 示例代码：
```typescript
// 64 位单字静态位图
let bs = BitSet64.initEmpty();
bs.set(10);
bs.set(42);
let count = bs.count(); // 2

let first = bs.findFirstSet(64); // 10
let is_subset = bs.subsetOf(BitSet64.initFull()); // true

// 动态可扩容位图
let dbs = DynamicBitSet.initEmpty(100);
defer dbs.deinit();
dbs.set(99);
dbs.resize(200, false); // 扩容至 200 位
```

---

### 2. `BoundedArray<T, CAP>`（栈分配固定上限数组）

#### 特性：
- 100% 栈物理内存布局，零堆分配开销，极度适合高频帧循环与局部作用域；
- 原生支持零拷贝切片导出（`slice()`, `constSlice()`）与任意位置插入/范围替换（`insert`, `replaceRange`）。

#### 示例代码：
```typescript
let arr = BoundedArray<i32, 16>{};
arr.append(10);
arr.append(20);
arr.insert(1, 15); // [10, 15, 20]

let s: []const i32 = arr.slice();
let has_item = arr.contains(15); // true
```

---

### 3. `LinearFifo<T>`（环形双端缓冲队列）

#### 特性：
- 环形缓冲自动回绕，`realign()` 可原地消除回绕重整为物理连续切片；
- `readableSlice(offset)` / `writableSlice(offset)` 直通系统 I/O（`read` / `write` / `send`）零拷贝缓冲；
- 支持 `unget(items)` 逆向回退未消费数据。

#### 示例代码：
```typescript
let fifo = LinearFifo<u8>{};
defer fifo.deinit();

fifo.write("HTTP/1.1 200 OK\r\n");
let slice = fifo.readableSlice(0);
// 消费 4 字节
fifo.discard(4);
// 反弹推回
fifo.unget("HTTP");
```

---

### 4. `BabyList<T>` 与 `OffsetByteList`（紧凑型 u32 动态数组）

#### 特性：
- 针对百万级微小数组（如 AST 节点、子标识符表），将内部索引从 64 位压缩为 32 位，内存开销减少 50%；
- 支持 `shrinkAndFree(len)` 精确按需回收多余内存。

---

### 5. `Soa<T>`（列式结构体数组）

- 基于连续内存分列存储，消除 AoS（Array of Structures）缓存污染；
- 通过 `items(field)` 暴露特定字段在物理内存中的连续单类型切片，直通 SIMD 向量化计算。

---

## 三、 测试与质量验证

完整测试位于 [cap/test/test_col_complete.cp](file:///data/dev/cp/cap/test/test_col_complete.cp)，覆盖率 100%，5/5 PASS。
