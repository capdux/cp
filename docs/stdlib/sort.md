# cap:sort — 高性能切片排序与对数检索标准库

> **模块协议**：`cap:sort`  
> **源码路径**：`lib/sort/index.cp`  
> **底层算法**：Zig Standard Library `std.sort`（Pattern-defeating Quicksort、BlockSort、HeapSort）

---

## 一、模块总览

`cap:sort` 为 CP 语言提供工业级零堆内存分配的内存切片原地排序与有序检索能力。其核心默认快速排序采用 Orson Peters 的 PDQSort（Pattern-defeating Quicksort），对已排序或大量重复元素切片具备 $O(N)$ 极致自适应性能，对最坏情况保证 $O(N \log N)$ 严格时间界，决不退化。

---

## 二、API 快速索引

| 函数/方法 | 时间复杂度 | 额外空间 | 算法/特性说明 |
| :--- | :--- | :--- | :--- |
| `Sort.asc(slice)` | $O(N) \sim O(N \log N)$ | $O(\log N)$ 栈空间 | 原地 PDQSort 升序排序，支持任意数值、字符串切片 |
| `Sort.desc(slice)` | $O(N) \sim O(N \log N)$ | $O(\log N)$ 栈空间 | 原地 PDQSort 降序排序 |
| `Sort.sort(slice, cmp)` | $O(N) \sim O(N \log N)$ | $O(\log N)$ 栈空间 | 原地 PDQSort 自定义比较函数 `cmp(a, b): bool` |
| `Sort.stable(slice, cmp)` | $O(N \log N)$ | $O(1)$ | 原地 BlockSort 稳定排序，保持相同键值元素的原始顺序 |
| `Sort.stableAsc(slice)` | $O(N \log N)$ | $O(1)$ | 原地 BlockSort 升序稳定排序 |
| `Sort.heap(slice, cmp)` | 严格 $O(N \log N)$ | 严格 $O(1)$ | 原地 HeapSort 原地堆排序，极端低内存场景首选 |
| `Sort.heapAsc(slice)` | 严格 $O(N \log N)$ | 严格 $O(1)$ | 原地 HeapSort 升序排序 |
| `Sort.isSorted(slice, cmp)` | $O(N)$ | $O(1)$ | 检查切片是否按照规则排序 |
| `Sort.isSortedAsc(slice)` | $O(N)$ | $O(1)$ | 检查切片是否升序排序 |
| `Sort.binarySearch(slice, key)` | $O(\log N)$ | $O(1)$ | 有序切片二分查找，返回元素下标 `usize?`，未找到返回 `null` |
| `Sort.lowerBound(slice, key)` | $O(\log N)$ | $O(1)$ | 返回首个不小于 `key` 的元素下标（插入位置下界） |
| `Sort.upperBound(slice, key)` | $O(\log N)$ | $O(1)$ | 返回首个大于 `key` 的元素下标（插入位置上界） |
| `Sort.minI32(slice)` / `maxI32` | $O(N)$ | $O(1)$ | 单趟遍历获取切片极小/极大值 |
| `Sort.argMin(slice)` / `argMax` | $O(N)$ | $O(1)$ | 单趟遍历获取切片极小/极大值对应的索引下标 |
| `SortOf<T>.min(slice)` / `max` | $O(N)$ | $O(1)$ | 泛型切片极小/极大值求值器 |

---

## 三、代码示例

### 1. 基础升序与降序排序
```typescript
import { test, expect_eq } from "cap:test";
import { Sort } from "cap:sort";

fn main() {
    let arr: [6]i32 = [42, -5, 18, 0, 99, 7];
    
    // 1. 升序排序
    Sort.asc(arr[0..]);
    expect_eq(arr[0], -5);
    expect_eq(arr[5], 99);
    expect_eq(Sort.isSortedAsc(arr[0..]), true);

    // 2. 降序排序
    Sort.desc(arr[0..]);
    expect_eq(arr[0], 99);
    expect_eq(arr[5], -5);
}
```

### 2. 自定义比较器排序
```typescript
import { Sort } from "cap:sort";

fn cmpDesc(a: i32, b: i32): bool {
    return a > b;
}

fn main() {
    let scores: [5]i32 = [78, 92, 85, 60, 99];
    Sort.sort(scores[0..], cmpDesc);
    // scores 此时变为 [99, 92, 85, 78, 60]
}
```

### 3. 稳定排序与堆排序
```typescript
import { Sort } from "cap:sort";

fn cmpAsc(a: i32, b: i32): bool {
    return a < b;
}

fn main() {
    // 稳定排序（BlockSort）
    let arr1: [5]i32 = [5, 2, 8, 2, 1];
    Sort.stable(arr1[0..], cmpAsc);

    // 严格 O(1) 辅助空间堆排序
    let arr2: [5]i32 = [99, 12, -4, 0, 3];
    Sort.heap(arr2[0..], cmpAsc);
}
```

### 4. 二分查找与区间边界定位
```typescript
import { test, expect_eq } from "cap:test";
import { Sort } from "cap:sort";

fn main() {
    let sorted_list: [7]i32 = [10, 20, 30, 30, 30, 40, 50];
    
    // 二分查找 (精确匹配)
    let found = Sort.binarySearch(sorted_list[0..], 40); // Some(5)
    let not_found = Sort.binarySearch(sorted_list[0..], 25); // null
    
    // 下界 (首个 >= 30 的索引)
    let lb = Sort.lowerBound(sorted_list[0..], 30); // 2
    
    // 上界 (首个 > 30 的索引)
    let ub = Sort.upperBound(sorted_list[0..], 30); // 5
    
    // 重复元素个数 = ub - lb = 3
    let count = ub - lb;
    expect_eq(count, 3);
}
```

### 5. 极值与极值索引定位
```typescript
import { Sort } from "cap:sort";

fn main() {
    let values: [5]i32 = [15, 3, 89, -12, 44];
    
    let min_val = Sort.minI32(values[0..]); // -12
    let max_val = Sort.maxI32(values[0..]); // 89
    
    let min_idx = Sort.argMin(values[0..]); // Some(3)
    let max_idx = Sort.argMax(values[0..]); // Some(2)
}
```
