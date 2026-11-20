# Soa<T> 高性能列式容器 (cap:col)

> 模块协议：`cap:col` 或 `cap:collections`  
> 源码路径：[`lib/col/soa.cp`](file:///data/dev/cp/lib/col/soa.cp)  
> 底层引擎：吸收自 Zig 官方原生 `std.MultiArrayList(T)`

---

## 一、 为什么需要 Soa<T>？（架构与性能本质）

在传统面向对象或普通数组中，数据通常采用 **AoS（Array of Structs，结构体数组）** 布局：
```
内存物理分布：[ {id, x, y, active}, {id, x, y, active}, {id, x, y, active}, ... ]
```
当渲染循环或物理引擎仅需要扫描 `x` 坐标时，CPU 从内存向 L1/L2 Cache 加载 64 字节 Cache Line，会把相邻不需要的 `id`、`y`、`active` 全部拖入缓存，导致 **CPU 缓存利用率低下（通常仅 25%~30%）**，且无法被 CPU SIMD 向量寄存器单指令批量吞吐。

**`Soa<T>`（Struct of Arrays，结构体数组化列式容器）** 则从底层重构内存分布：
```
ids:     [ 1,     2,     3,     ... ]
xs:      [ 10.0,  20.0,  30.0,  ... ]  <-- 纯物理连续数组！100% 榨干 CPU L1 缓存行！
ys:      [ 0.0,   5.0,   8.0,   ... ]
actives: [ true,  true,  false, ... ]
```
每个字段在底层对应一块独立、纯粹的连续内存切片，具备以下质的飞跃：
1. **CPU 缓存命中率逼近 100%**：单字段批量扫描时零伪共享、零冗余字段污染；
2. **直通硬件 SIMD 向量化**：直接将单列切片加载至 AVX2 / AVX-512 / ARM NEON 寄存器进行并行加乘规约；
3. **消除字段对齐填充碎片**：不同字段分别紧凑排列，省去传统 AoS 中因内存对齐产生的空洞 Padding。

---

## 二、 API 矩阵与方法概览

```typescript
import { Soa } from "cap:col";

export class Soa<T> {
    // 构造与析构
    pub static fn init(allocator: auto = null): Soa<T>;
    pub static fn initCapacity(initial_cap: usize, allocator: auto = null): Soa<T>;
    pub fn deinit(*_);

    // 增删改查
    pub fn append(*_, item: T);
    pub fn get(_, index: usize): T;
    pub fn set(*_, index: usize, item: T);
    pub fn popOrNull(*_): T?;
    pub fn pop(*_): T?;
    pub fn swapRemove(*_, index: usize);
    pub fn orderedRemove(*_, index: usize);

    // 容量与状态
    pub fn len(_): usize;
    pub fn capacity(_): usize;
    pub fn isEmpty(_): bool;
    pub fn clearRetainingCapacity(*_);

    // 核心高阶列切片提取（直通 SIMD）
    pub fn items(*_, comptime field: any): auto;
}
```

---

## 三、 典型代码示例

### 1. 基础用法
```cp
import { Soa } from "cap:col";

struct Particle {
    id: u32,
    x: f32,
    y: f32,
    active: bool,
}

fn main() {
    let mut particles = Soa<Particle>.init();
    defer particles.deinit();

    // 追加粒子
    particles.append(Particle{ id: 1, x: 10.0, y: 20.0, active: true });
    particles.append(Particle{ id: 2, x: 30.0, y: 40.0, active: true });

    // 按行读取与修改
    let p = particles.get(0);
    particles.set(1, Particle{ id: 2, x: 35.0, y: 45.0, active: false });
}
```

### 2. 核心大招：按列提取连续切片并执行 SIMD 向量计算
```cp
import { Soa } from "cap:col";

fn computeSum(particles: *Soa<Particle>): f32 {
    // 零拷贝提取 x 坐标物理列切片（类型为 []f32）
    let xs: []f32 = particles.items(.x);

    let mut sum: f32 = 0.0;
    // 纯连续内存单循环，编译器极易将其自动向量化
    for (x of xs) {
        sum += x;
    }
    return sum;
}
```

---

## 四、 最佳适用场景

- **游戏引擎与物理模拟（ECS 架构）**：Entity-Component-System 中组件数组列式聚合；
- **网页与 UI 渲染树（DOM 节点池）**：批量计算千万级 `Rect` 盒模型坐标；
- **大数据流与点击流分析**：单列聚合统计（如 `SUM(amount)`、`AVG(duration)`）；
- **AI Agent 与向量检索**：高维 Embedding 矩阵与离散特征快速过滤。
