# CP 标准库：高性能零 GC 容器与数据结构子系统使用手册 (`cap:col`)

## 一、 架构总览与设计哲学

`cap:col`（兼容映射 `cap:collections`）是 CP 语言专为**百万级 AI Agent 并发、确定性无 GC 内存管理与 CPU 缓存行局部性（Cache-Line Locality）**设计的工业级数据结构标准库。该子系统完整吸收自 Bun（`/data/dev/bun/src/collections/`）与 Zig 高性能容器原语，消除了传统动态语言中链表指针追逐（Pointer Chasing）与垃圾回收停顿的开销。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                            cap:col 高性能数据结构全家桶                                │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   零堆分配栈内/定长容器    │    动态紧凑/面向数据容器     │   树、堆、位图与紧凑编码引擎  │
│  • BoundedArray<T, CAP>   │  • BabyList<T> (u32 索引)   │  • BitSet64..1024 / Dynamic  │
│  • HiveArray<T, CAP>      │  • OffsetByteList (流游标)  │  • PriorityQueue<T> (最小堆) │
│  • ObjectPool<T, CAP>     │  • LinearFifo<T> (2^N 环队) │  • Treap<K, V> (随机平衡树)  │
│  • StaticHashMap<K,V,CAP> │  • Soa<T> (Struct of Arrays)│  • Leb128 (ULEB128/SLEB128)  │
│  • FastStringMap<V, CAP>  │                             │                              │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

### 核心内存布局解析

1. **紧凑索引与缓存行亲和（`BabyList<T>` vs 标准 Vector）**：
   - 标准 64 位动态数组需要 `ptr (8B) + len (8B) + cap (8B) = 24 字节`；
   - `BabyList<T>` 采用 `u32` 维护 `len` 与 `cap`（最大支持 40 亿元素），在 AST 节点、CSS 属性列表、打包器符号表等海量小列表场景下大幅压缩元数据体积。
2. **硬件指令加速槽位分配（`HiveArray<T, CAP>` & `BitSet64`）**：
   - `HiveArray` 利用 64 位位图掩码（`BitSet64`）记录槽位占用状态，通过单条 CPU 硬件尾零计数指令 `@ctz(~mask)` 在严格 **O(1)** 周期内定位空闲槽位，且保证已分配元素索引在增删过程中 100% 稳定。
3. **面向数据设计（`Soa<T>` — Structure of Arrays）**：
   - 将传统的 `AoS`（Array of Structs，`[ {x, y, z}, {x, y, z} ]`）转置为按字段连续排列的列式内存块（`[x0, x1...], [y0, y1...], [z0, z1...]`），消除结构体对齐填充浪费，并直接赋能 AVX2 / NEON SIMD 向量化批处理。

---

## 二、 十二大容器时间/空间复杂度与选型矩阵

| 容器 / 结构 | 底层物理布局 | 插入 / 分配 | 查找 / 访问 | 删除 / 释放 | 堆分配次数 | 典型适用场景 |
| :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **`BabyList<T>`** | 连续动态数组 (`u32` len/cap) | 均摊 $O(1)$ | $O(1)$ | 尾部 $O(1)$ / 保序 $O(N)$ | 按需倍增 | AST 节点列表、高频紧凑动态数组 |
| **`OffsetByteList`** | `BabyList<u8>` + `u32` 读游标 | 均摊 $O(1)$ | $O(1)$ | 游标推进 $O(1)$ | 按需倍增 | 网络发包缓冲、流式字节消费 |
| **`HiveArray<T, CAP>`** | 定长数组 + `BitSet64` 掩码 | 严格 $O(1)$ (`@ctz`) | 严格 $O(1)$ | 严格 $O(1)$ | **0（栈内零堆）** | 并发连接槽、句柄表（上限 $\le 64$） |
| **`ObjectPool<T, CAP>`** | 定长数组 + 侵入式索引空闲链表 | 严格 $O(1)$ | 严格 $O(1)$ | 严格 $O(1)$ | **0（栈内零堆）** | 任意容量零碎片对象池、任务节点池 |
| **`BoundedArray<T, CAP>`**| 栈内连续定长缓冲区 `[CAP]T` | 严格 $O(1)$ | 严格 $O(1)$ | 尾部 $O(1)$ / `swapRemove` $O(1)$ | **0（栈内零堆）** | 路径拼接、小参数列表、热路径局部缓冲 |
| **`LinearFifo<T>`** | $2^N$ 容量环形缓冲区 (位掩码寻址)| 均摊 $O(1)$ | $O(1)$ | 头部弹出 $O(1)$ | 按需倍增 | 异步 I/O 队列、任务调度 FIFO、字节环 |
| **`PriorityQueue<T>`** | 连续数组二叉最小堆 | $O(\log N)$ | 堆顶 $O(1)$ | 弹出最小元 $O(\log N)$ | 按需倍增 | 定时器最小堆、Dijkstra / A* 调度 |
| **`BitSet64..1024` / `DynamicBitSet`** | `[N]u64` 字数组 (`@ctz`/`@popCount`)| 严格 $O(1)$ | 严格 $O(1)$ | 严格 $O(1)$ | 静态 **0** / 动态 1 | 权限掩码、活跃 ID 集合、布隆/位图过滤 |
| **`Treap<K, V>`** | 1-based 索引池 + SplitMix64 旋转树 | 平均 $O(\log N)$ | 平均 $O(\log N)$| 平均 $O(\log N)$ | **0（内联节点池）**| 有序键值映射、最小/最大键实时查询 |
| **`StaticHashMap<K,V,CAP>`**| 开放寻址线性探测 + Wyhash | 平均 $O(1)$ | 平均 $O(1)$ | 平均 $O(1)$ (簇重哈希) | **0（栈内零堆）** | 零堆分配高速键值缓存（`CAP` 为 $2^N$） |
| **`FastStringMap<V, CAP>`**| 长度预过滤 (`key.len`) + 字节比对 | $O(N)$ | $O(N)$ (极速剪枝)| $O(1)$ 清空 | **0（栈内零堆）** | HTTP Header、配置键、大小写不敏感字典 |
| **`Soa<T>`** | 列式分离连续内存 (`MultiArrayList`)| 均摊 $O(1)$ | 单列切片 $O(1)$ | `swapRemove` $O(1)$ | 按需列式倍增 | ECS 游戏/物理引擎、SIMD 向量统计分析 |
| **`Leb128`** | 7-bit 变长字节流 (MSB 延续位) | 编码 $O(1..10\text{B})$ | 解码 $O(1..10\text{B})$ | — | **0（零分配）** | WebAssembly 字节码、DWARF、RPC 紧凑整型 |

---

## 三、 十二大核心组件详解与完整代码示例

### 1. `BabyList<T>` 与 `OffsetByteList`（超紧凑 `u32` 动态列表）

源自 `/data/dev/bun/src/collections/baby_list.cp`。支持 `fromSlice`、`append`、`appendSlice`、`insert`、`orderedRemove`、`swapRemove`、`contains`、`findIndex`、`clone`、`shrinkAndFree` 等全套生命周期管理；配套的 `OffsetByteList` 提供带 `head` 游标的流式字节缓冲，当全部消费完毕时自动归零复用底层容量：

```typescript
import { BabyList, OffsetByteList } from "cap:col";

pub fn demoBabyList() {
    let list = BabyList<i32>.fromSlice([10, 20, 30]);
    defer list.release();

    list.append(40);
    list.insert(1, 15); // [10, 15, 20, 30, 40]
    const removed = list.orderedRemove(2); // 移除 20
    const last_opt = list.pop();           // 弹出 40

    // OffsetByteList: 带消费游标的字节流缓冲区
    let obl = OffsetByteList.init(64);
    defer obl.release();
    obl.write("HTTP/1.1 200 OK\r\n");
    const consumed = obl.consume(9); // 消费 "HTTP/1.1 "，剩余 "200 OK\r\n"
    const rem = obl.remaining();     // "200 OK\r\n"
}
```

---

### 2. `HiveArray<T, CAP>`（O(1) 硬件位指令加速蜂巢槽位池）

源自 `/data/dev/bun/src/collections/hive_array.cp`。底层使用 `BitSet64` 追踪空闲槽位，通过 `@ctz` 在常数时间内分配稳定索引句柄：

```typescript
import { HiveArray } from "cap:col";

pub fn demoHiveArray() {
    let hive = HiveArray<i64, 8>.init();

    const s0 = hive.acquire(1001).?;
    const s1 = hive.acquire(1002).?;
    const s2 = hive.acquire(1003).?;

    // 释放中间槽位 s1，不影响 s0 与 s2 的索引有效性
    hive.releaseSlot(s1);

    // 再次申请时，O(1) 立即复用刚刚释放的 s1 槽位
    const s_reused = hive.acquire(9999).?;
}
```

---

### 3. `ObjectPool<T, CAP>`（零堆分配侵入式空闲链表对象池）

源自 `/data/dev/bun/src/collections/pool.cp`。突破 64 槽位限制，支持任意编译期容量 `CAP`，通过栈内 `next_free` 索引链表实现严格 $O(1)$ 的 `acquire` 与 `releaseSlot`：

```typescript
import { ObjectPool } from "cap:col";

pub fn demoObjectPool() {
    let pool = ObjectPool<i32, 128>.init();

    const id_a = pool.acquire(42).?;
    const id_b = pool.acquire(84).?;
    pool.set(id_a, 100);

    const val_a = pool.get(id_a).?; // 100
    pool.releaseSlot(id_a);         // O(1) 归还至空闲链表头
}
```

---

### 4. `BoundedArray<T, CAP>`（栈内定长边界动态数组）

源自 `/data/dev/bun/src/collections/bounded_array.cp`。100% 存储在栈帧或父结构体内联内存中，无任何堆分配或指针解引用，支持 `append`、`insertSlice`、`replaceRange`、`swapRemove`、`orderedRemove`：

```typescript
import { BoundedArray } from "cap:col";

pub fn demoBoundedArray() {
    let arr = BoundedArray<i32, 8>.fromSlice([1, 2, 3, 4]);
    arr.append(5);
    arr.replaceRange(1, 2, [20, 30, 35]); // 将 [2, 3] 替换为 [20, 30, 35]
    const idx = arr.findIndex(30).?;
}
```

---

### 5. `LinearFifo<T>`（$2^N$ 容量环形缓冲队列）

源自 `/data/dev/bun/src/collections/linear_fifo.cp`。容量自动向上取整为 2 的幂次，采用位与掩码 `(head + offset) & (cap - 1)` 替代昂贵的取模运算，并支持 `unget`（头部回退压入）与 `realign`（内存线性对齐）：

```typescript
import { LinearFifo } from "cap:col";

pub fn demoLinearFifo() {
    let fifo = LinearFifo<i32>.init(8);
    defer fifo.release();

    fifo.writeSlice([10, 20, 30]);
    fifo.unget([5]); // 头部回插 -> 队列变为 [5, 10, 20, 30]

    const first = fifo.readItem().?; // 5
    let out: [2]i32 = [0, 0];
    const n = fifo.readSlice(out[0..2]); // 读取 [10, 20]
}
```

---

### 6. `PriorityQueue<T>`（二叉最小堆优先队列）

基于连续内存数组的二叉最小堆，每次 `pop()` 保证以 $O(\log N)$ 时间复杂度弹出当前最小元素，`peek()` 以 $O(1)$ 查看堆顶：

```typescript
import { PriorityQueue } from "cap:col";

pub fn demoPriorityQueue()! {
    let pq = PriorityQueue<i32>.init();
    defer pq.deinit();

    pq.push(50);
    pq.push(10);
    pq.push(30);
    try pq.add(5);

    const top = pq.peek().?; // 5 (不弹出)
    const p1 = pq.pop().?;   // 5
    const p2 = pq.pop().?;   // 10
    const p3 = pq.pop().?;   // 30
    const p4 = pq.pop().?;   // 50
}
```

---

### 7. `BitSet` 系列（`BitSet64` / `128` / `256` / `512` / `1024` & `DynamicBitSet`）

源自 `/data/dev/bun/src/collections/bit_set.cp`。提供编译期定长位图与运行时可动态扩容（`resize`）的 `DynamicBitSet`，全面支持集合并集（`setUnion`）、交集（`setIntersection`）、对称差（`xorWith`）、补集（`complement`）、区间批量置位（`setRangeValue`）及零分配位迭代器（`BitSet64Iter` / `BitSet256Iter`）：

```typescript
import { BitSet64, BitSet256, DynamicBitSet, Range } from "cap:col";

pub fn demoBitSet() {
    let b256 = BitSet256.initEmpty();
    b256.set(3);
    b256.set(128);
    b256.setRangeValue(Range.init(200, 210), true);
    const total_bits = b256.count();       // 12
    const first_bit = b256.findFirstSet().?; // 3

    // 动态位图
    let dbs = DynamicBitSet.initEmpty(100);
    defer dbs.release();
    dbs.set(95);
    dbs.resize(300, false);
    dbs.set(250);
}
```

---

### 8. `Treap<K, V>`（树堆：SplitMix64 随机平衡二叉搜索树）

将二叉搜索树（按 `Key` 严格有序）与最大堆（按 SplitMix64 高熵伪随机 `priority` 维护堆序）结合，通过左旋（`rotateLeft`）与右旋（`rotateRight`）维持期望深度 $O(\log N)$，且采用 1-based 内联节点池彻底消灭指针碎片：

```typescript
import { Treap } from "cap:col";

pub fn demoTreap() {
    let tree = Treap<i64, i64>.init();

    tree.insert(50, 500);
    tree.insert(20, 200);
    tree.insert(80, 800);
    tree.insert(35, 350);

    const v = tree.get(35).?;      // 350
    const min_k = tree.minKey().?; // 20
    const max_k = tree.maxKey().?; // 80

    tree.remove(20);               // O(log N) 旋转下沉删除后，minKey 变为 35
}
```

---

### 9. `StaticHashMap<K, V, CAP>` 与 `FastStringMap<V, CAP>`（开放寻址与长度过滤哈希表）

源自 `/data/dev/bun/src/collections/StaticHashMap.cp` 与 `comptime_string_map.cp`：

- **`StaticHashMap<K, V, CAP>`**：支持任意整型与字符串键（内置 `Wyhash` 与黄金比例混淆），采用开放寻址线性探测与删除后簇重哈希（Cluster Rehashing），要求 `CAP` 为 2 的幂次；
- **`FastStringMap<V, CAP>`**：在进行字节比对前先按 `key.len` 进行寄存器级过滤，同时支持精确匹配 `get(key)` 与 ASCII 大小写不敏感匹配 `getCaseInsensitive(key)`。

```typescript
import { StaticHashMap, FastStringMap } from "cap:col";

pub fn demoMaps() {
    // 1. 零堆分配开放寻址哈希表 (CAP = 16)
    let hmap = StaticHashMap<i64, i64, 16>.init();
    hmap.put(101, 2026);
    hmap.put(202, 4096);
    const val = hmap.get(101).?; // 2026
    hmap.delete(101);

    // 2. 长度预过滤快速字符串字典（支持大小写不敏感查询）
    let smap = FastStringMap<i32, 16>.init();
    smap.put("Content-Type", 1);
    smap.put("Authorization", 2);

    const exact = smap.get("Content-Type").?;                // 1
    const ci = smap.getCaseInsensitive("content-type").?;    // 1
}
```

---

### 10. `Soa<T>`（Structure of Arrays 列式面向数据容器）

源自 `/data/dev/bun/src/collections/multi_array_list.cp`（底层由 `std.MultiArrayList(T)` 驱动）。支持按行追加/读取完整结构体，同时支持通过 `.items(.field_name)` 提取任意单列的连续内存切片进行 SIMD 向量化计算：

```typescript
import { Soa } from "cap:col";

pub struct Particle {
    x: f32 = 0.0,
    y: f32 = 0.0,
    mass: i32 = 1,
}

pub fn demoSoa() {
    let particles = Soa<Particle>.initCapacity(16);
    defer particles.deinit();

    particles.append(Particle{ x: 1.5, y: 2.5, mass: 10 });
    particles.append(Particle{ x: 3.5, y: 4.5, mass: 20 });

    const p0 = particles.get(0);
    // 提取连续的 mass 列切片 ([]i32) 进行零跨距遍历或 SIMD 归约
    let masses: []i32 = particles.items(.mass);
    const total_mass = masses[0] + masses[1]; // 30
}
```

---

### 11. `Leb128`（无符号 ULEB128 与有符号 SLEB128 变长整数编解码）

`lib/col/leb128.cp` 提供符合 DWARF / WebAssembly 标准规范的无符号 `ULEB128` 与有符号二补数 `SLEB128` 编解码器，以及编码字节数预计算函数 `uleb128Size` / `sleb128Size`：

```typescript
import { Leb128 } from "cap:col";

pub fn demoLeb128() {
    let buf: [16]u8 = [0; 16];

    // 1. 无符号 ULEB128 编解码 (例如 DWARF 标准向量 624485 -> [0xE5, 0x8E, 0x26])
    const u_written = Leb128.encodeUleb128(624485, buf[0..16]);
    if (u_res := Leb128.decodeUleb128(buf[0..u_written])) {
        @print("ULEB128 解码值: {d}, 消耗字节: {d}\n", u_res.value, u_res.len);
    }

    // 2. 有符号 SLEB128 编解码 (例如负数 -123456)
    const s_written = Leb128.encodeSleb128(-123456, buf[0..16]);
    if (s_res := Leb128.decodeSleb128(buf[0..s_written])) {
        @print("SLEB128 解码值: {d}, 消耗字节: {d}\n", s_res.value, s_res.len);
    }
}
```
