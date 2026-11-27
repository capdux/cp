[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# damerau-levenshtein

面向 CP 语言的零堆分配 Damerau-Levenshtein（最优字符串对齐 OSA 与真 Damerau-Levenshtein）编辑距离及相似度计算库。

对齐 npm 源包 [`damerau-levenshtein`](https://github.com/tad-lispy/node-damerau-levenshtein) `1.0.8`（MIT）。

## 安装与导入

```bash
cap add damerau-levenshtein
```

```cp
import {
    DamLevResult,
    DL_MAX_OSA_LEN,
    DL_MAX_TRUE_LEN,
    damerauLevenshtein,
    damerauLevenshteinSteps,
    damerauLevenshteinBounded,
    trueDamerauLevenshtein,
    similarity,
    trueSimilarity,
} from "damerau-levenshtein";
```

## 核心 API

### 常量与数据结构

| 符号 | 类型 / 字段 | 说明 |
| --- | --- | --- |
| `DL_MAX_OSA_LEN` | `usize = 255` | 栈上 OSA 动态规划缓冲区支持的最大字节长度。 |
| `DL_MAX_TRUE_LEN` | `usize = 64` | 栈上真 Damerau-Levenshtein 动态规划缓冲区支持的最大字节长度。 |
| `DamLevResult` | `{ steps: usize, relative: f64, similarity: f64 }` | 编辑距离结果（`relative = steps / maxLen`，`similarity = 1.0 - relative`）。 |

### 函数列表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `damerauLevenshtein` | `(a: string, b: string): DamLevResult` | 计算 OSA 编辑步数、相对距离与相似度。 |
| `damerauLevenshteinSteps` | `(a: string, b: string): usize` | 计算字符串 OSA 编辑步数。 |
| `damerauLevenshteinStepsBytes` | `(a: []const u8, b: []const u8): usize` | 计算只读字节切片的 OSA 编辑步数。 |
| `osaDistance` | `(a: string, b: string): usize` | `damerauLevenshteinSteps` 别名。 |
| `osaDistanceBytes` | `(a: []const u8, b: []const u8): usize` | `damerauLevenshteinStepsBytes` 别名。 |
| `trueDamerauLevenshtein` | `(a: string, b: string): DamLevResult` | 计算允许对换位子串继续编辑的真 Damerau-Levenshtein 完整结果。 |
| `trueDamerauLevenshteinSteps` | `(a: string, b: string): usize` | 计算字符串真 Damerau-Levenshtein 步数。 |
| `trueDamerauLevenshteinBytes` | `(a: []const u8, b: []const u8): usize` | 计算字节切片真 Damerau-Levenshtein 步数。 |
| `damerauLevenshteinBounded` | `(a: string, b: string, maxDistance: usize): usize?` | 带上限提前剪枝的 OSA 距离，超过 `maxDistance` 返回 `null`。 |
| `damerauLevenshteinBoundedBytes` | `(a: []const u8, b: []const u8, maxDistance: usize): usize?` | 字节切片版带上限 OSA 距离，超限返回 `null`。 |
| `similarity` | `(a: string, b: string): f64` | 归一化 OSA 相似度，范围 `[0.0, 1.0]`。 |
| `trueSimilarity` | `(a: string, b: string): f64` | 归一化真 Damerau-Levenshtein 相似度，范围 `[0.0, 1.0]`。 |

## 使用示例

### 1. 基础编辑距离与相似度

```cp
import { print } from "cap:io";
import { damerauLevenshtein, similarity } from "damerau-levenshtein";

pub fn main() {
    let res = damerauLevenshtein("kitten", "sitting");
    print("steps={}, relative={}, similarity={}\n", res.steps, res.relative, res.similarity);

    let score = similarity("abcdef", "abcfed");
    print("similarity={}\n", score);
}
```

### 2. 带上限剪枝的模糊匹配

```cp
import { print } from "cap:io";
import { damerauLevenshteinBounded } from "damerau-levenshtein";

pub fn main() {
    let maybe = damerauLevenshteinBounded("recieve", "receive", 2);
    if (maybe != null) {
        print("命中模糊匹配，编辑步数={}\n", maybe!);
    } else {
        print("超出编辑距离阈值\n");
    }
}
```

### 3. OSA 与真 Damerau-Levenshtein 对比

```cp
import { print } from "cap:io";
import { osaDistance, trueDamerauLevenshteinSteps } from "damerau-levenshtein";

pub fn main() {
    // "ca" -> "abc": OSA = 3, True Damerau-Levenshtein = 2
    let osa = osaDistance("ca", "abc");
    let true_dl = trueDamerauLevenshteinSteps("ca", "abc");
    print("OSA={}, True={}\n", osa, true_dl);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：全部 DP 矩阵位于固定大小栈数组（`DL_MAX_OSA_LEN = 255`、`DL_MAX_TRUE_LEN = 64`），无 GC 与堆分配开销。
- **双算法与剪枝扩展**：除对齐 npm 原包的 OSA 算法外，额外提供真 Damerau-Levenshtein（`trueDamerauLevenshtein*`）以及提前退出阈值版（`damerauLevenshteinBounded*`）。
- **只读借用安全**：所有字符串和字节切片均通过不可变借用读取，零拷贝且无所有权转移负担。
