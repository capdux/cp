# `cap:sort` — 工业级自适应排序（PDQSort / BlockSort / HeapSort）与二分检索标准库

> 参考来源：`/data/dev/bun/src/collections/`, Zig Standard Library `std.sort` | 吸收日期：2026-11-22
> **模块路径**: `lib/sort/`  
> **导入协议**: `import { Sort, SortOf } from "cap:sort";`

---

## 一、 架构概述与算法原理

`cap:sort` 提供了**零动态堆分配（Zero-Heap Allocation）、原地执行（In-Place）**的三大工业级排序引擎与全套有序切片二分查询、极值归约原语：

1. **PDQSort（Pattern-Defeating Quicksort，自适应模式击败快速排序 — `Sort.asc`, `Sort.desc`, `Sort.sort`）**：
   - 结合了三数取中/九数取中快速排序（Quicksort）、小数组插入排序（Insertion Sort）与堆排序兜底（Heapsort Fallback）。
   - **抗退化与模式识别**：当检测到输入序列已接近有序、逆序或包含大量重复相等元素（三路划分 Equal Partitioning）时，自动切换至线性扫描路径，实现 **$O(N)$ 最佳时间复杂度**；当递归深度超过 $2 \log_2 N$ 或枢轴选择不平衡时，自动回退至堆排序，严格保证 **$O(N \log N)$ 最坏时间复杂度**（彻底免疫针对传统快排的二次方复杂度退化）。
2. **BlockSort（原地稳定块排序 — `Sort.stable`, `Sort.stableAsc`）**：
   - 在完全不分配外部堆内存（$O(1)$ 辅助空间）的前提下，通过内部缓冲块交换与归并，严格保持相等元素的原始相对顺序（Stable Sort），保证 $O(N \log N)$ 最坏时间复杂度。
3. **HeapSort（原地二叉堆排序 — `Sort.heap`, `Sort.heapAsc`）**：
   - 适用于栈空间极端受限场景，$O(1)$ 严格常数空间开销（无递归栈）与 $O(N \log N)$ 确定性时间上界。
4. **有序切片二分上下界搜索与极值定位**：
   - 提供 `binarySearch`（精确二分查找）、`lowerBound`（首个 $\ge \text{key}$ 的插入索引）、`upperBound`（首个 $> \text{key}$ 的插入索引）、`isSorted` / `isSortedAsc`（有序性线性校验）以及 `argMin` / `argMax` / `minI32` / `maxI32` 等极值提取器。

---

## 二、 算法时间与空间复杂度矩阵

| 方法名称 | 底层算法 | 最佳时间复杂度 | 平均时间复杂度 | 最坏时间复杂度 | 辅助空间复杂度 | 稳定性 (Stable) |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| `Sort.asc` / `Sort.desc` / `Sort.sort` | **PDQSort** | $O(N)$ | $O(N \log N)$ | $O(N \log N)$ | $O(\log N)$ 栈 | 否 (Unstable) |
| `Sort.stable` / `Sort.stableAsc` | **BlockSort** | $O(N)$ | $O(N \log N)$ | $O(N \log N)$ | $O(1)$ | **是 (Stable)** |
| `Sort.heap` / `Sort.heapAsc` | **HeapSort** | $O(N \log N)$ | $O(N \log N)$ | $O(N \log N)$ | $O(1)$ | 否 (Unstable) |
| `Sort.binarySearch` | 二分折半查找 | $O(1)$ | $O(\log N)$ | $O(\log N)$ | $O(1)$ | — |
| `Sort.lowerBound` / `upperBound` | 二分边界搜索 | $O(\log N)$ | $O(\log N)$ | $O(\log N)$ | $O(1)$ | — |
| `Sort.isSorted` / `isSortedAsc` | 线性单调校验 | $O(1)$ | $O(N)$ | $O(N)$ | $O(1)$ | — |
| `Sort.argMin` / `argMax` / `SortOf<T>` | 线性极值扫描 | $O(N)$ | $O(N)$ | $O(N)$ | $O(1)$ | — |

---

## 三、 核心 API 汇总表

| 分类 | API 签名 | 功能说明 |
| :--- | :--- | :--- |
| **PDQSort 快速排序** | `Sort.asc(slice)` / `Sort.desc(slice)` | 原地升序 / 降序 PDQSort |
| **自定义比较器快排** | `Sort.sort(slice, cmp_fn)` | 使用自定义函数 `fn(a: T, b: T): bool` 进行原地 PDQSort |
| **稳定排序** | `Sort.stableAsc(slice)` / `Sort.stable(slice, cmp_fn)` | 保持相等键原始先后顺序的原地稳定排序 |
| **堆排序** | `Sort.heapAsc(slice)` / `Sort.heap(slice, cmp_fn)` | 零递归、$O(1)$ 辅助空间的原地堆排序 |
| **有序校验** | `Sort.isSortedAsc(slice)` / `Sort.isSorted(slice, cmp_fn)` | 判断切片是否已按指定顺序排列 |
| **二分搜索** | `Sort.binarySearch(slice, key): usize?` | 在升序切片中二分查找等于 `key` 的元素索引，未找到返回 `null` |
| **二分下界 (`>=`)** | `Sort.lowerBound(slice, key): usize` | 返回升序切片中**第一个不小于 (`>=`) `key`** 的索引位置 |
| **二分上界 (`>`)** | `Sort.upperBound(slice, key): usize` | 返回升序切片中**第一个严格大于 (`>`) `key`** 的索引位置 |
| **极值与索引** | `Sort.argMin(slice)` / `Sort.argMax(slice)` | 返回最小 / 最大元素的下标索引 `usize?` |
| **标量极值快捷函数** | `Sort.minI32` / `maxI32` / `minI64` / `maxI64` / `minF64` / `maxF64` | 返回对应类型切片的最小值或最大值 `T?` |

---

## 四、 代码示例

### 1. PDQSort 升序/降序、自定义比较器与稳定排序

```typescript
import { Sort } from "cap:sort";

fn cmpAbsAsc(a: i32, b: i32): bool {
    const abs_a = if (a < 0) -a else a;
    const abs_b = if (b < 0) -b else b;
    return abs_a < abs_b;
}

fn demoSorting(): void {
    let nums: [6]i32 = [42, -5, 18, 0, 99, 7];

    // 1. 原地升序 PDQSort
    Sort.asc(nums[0..]);
    // nums => [-5, 0, 7, 18, 42, 99]
    let ok = Sort.isSortedAsc(nums[0..]); // true

    // 2. 原地降序 PDQSort
    Sort.desc(nums[0..]);
    // nums => [99, 42, 18, 7, 0, -5]

    // 3. 使用自定义比较器（按绝对值升序）进行稳定排序 BlockSort
    Sort.stable(nums[0..], cmpAbsAsc);
    // nums => [0, -5, 7, 18, 42, 99]
}
```

### 2. 二分精确查找与 `lowerBound` / `upperBound` 区间频次查询

利用 `upperBound(slice, key) - lowerBound(slice, key)`，可在 **$O(\log N)$ 对数时间**内统计有序数组中某个元素的重复出现次数或切片范围：

```typescript
import { Sort } from "cap:sort";

fn demoBinaryBounds(): void {
    // 已按升序排列的延迟采样数组（含重复值 30）
    let latencies: [7]i32 = [10, 20, 30, 30, 30, 40, 50];

    // 1. 精确二分查找
    if (idx := Sort.binarySearch(latencies[0..], 40)) {
        // idx => 5
    }

    // 2. lowerBound (首个 >= 30 的位置) 与 upperBound (首个 > 30 的位置)
    const lb = Sort.lowerBound(latencies[0..], 30); // lb => 2
    const ub = Sort.upperBound(latencies[0..], 30); // ub => 5

    // 值 30 在数组中的出现次数为 ub - lb = 3 次，对应子切片 latencies[lb..ub]
    const count_30 = ub - lb; // 3
}
```

### 3. 极值与极值下标定位 (`minI32`, `maxI32`, `argMin`, `argMax`)

```typescript
import { Sort } from "cap:sort";

fn demoExtrema(): void {
    let scores: [5]i32 = [85, 92, 60, 99, 74];

    const min_val = Sort.minI32(scores[0..]).?; // 60
    const max_val = Sort.maxI32(scores[0..]).?; // 99

    const min_idx = Sort.argMin(scores[0..]).?; // 2 (scores[2] == 60)
    const max_idx = Sort.argMax(scores[0..]).?; // 3 (scores[3] == 99)
}
```

---

## 上游行为契约与边界注意事项

1. **严格弱序（Strict Weak Ordering）契约**：
   - 传给 `Sort.sort` / `Sort.stable` / `Sort.heap` 的自定义比较器必须满足反自反性（`cmp(a, a) == false`）与传递性；**排序浮点切片时严禁包含 `NaN`**（`NaN < x` 与 `x < NaN` 均为 `false` 会破坏严格弱序），调用前应先过滤或归一化 `NaN`。
2. **稳定性与栈深度选型**：
   - 多键排序或需要保留原始输入次序时，必须使用 `Sort.stable` / `Sort.stableAsc`（原地 BlockSort，$O(1)$ 辅助空间）；在极小栈空间（如嵌入式/微协程）环境下使用 `Sort.heap` / `Sort.heapAsc` 可完全消除递归栈开销。
3. **二分查询前置条件与空切片安全**：
   - `Sort.binarySearch`、`Sort.lowerBound`、`Sort.upperBound` 要求输入切片已按升序排列（可用 `Sort.isSortedAsc` 校验）；`SortOf<T>.min` / `max` 与 `Sort.argMin` / `argMax` 在空切片输入时安全返回 `null`。
