[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# damerau-levenshtein

> **源包溯源**：吸收自 **npm** [`damerau-levenshtein`](https://github.com/tad-lispy/node-damerau-levenshtein)（`v1.0.8` / `MIT`），CP 官方洁净室实现 `damerau-levenshtein@1.0.8`。

纯 CP 实现的零堆分配 Damerau-Levenshtein（最优字符串对齐，OSA）编辑距离与相似度计算库。

## 核心亮点

- **零堆分配 3 行滚动 DP**：仅使用 3 个固定栈行（`[128]usize`），彻底避免分配 `O(m × n)` 二维堆矩阵。
- **零栈清零优化**：3 个滚动 DP 数组均采用 `undefined` 初始化且严格先写后读，消除每次调用 3KB 的冗余栈清零开销。
- **支持相邻字符换位**：除插入、删除、替换外，将相邻字符颠倒（如 `"ca"` ↔ `"ac"`）计为 1 步，非常适合 CLI 命令拼写纠错、模糊搜索与标识符推荐。

## 零成本迁移（TS → CP）

直接沿用原包名 `damerau-levenshtein`，无需改名：

**TypeScript：**
```typescript
import levenshtein from "damerau-levenshtein";
const res = levenshtein("kitten", "sitting");
```

**CP：**
```typescript
import {
    damerauLevenshtein, damerauLevenshteinSteps, DamLevResult
} from "damerau-levenshtein";
```

## 实战示例

```typescript
import { damerauLevenshtein, damerauLevenshteinSteps } from "damerau-levenshtein";

export fn demoFuzzyMatch(): bool {
    // 仅计算步数（无浮点运算）
    let steps = damerauLevenshteinSteps("ca", "ac"); // 1（相邻换位）

    // 计算步数、相对距离与相似度
    let res = damerauLevenshtein("commit", "comimt");
    // res.steps == 1, res.relative == 1.0/6.0, res.similarity == 5.0/6.0
    return steps == 1 && res.similarity > 0.8;
}
```

## 核心 API 速览

- **`DamLevResult`**：`steps: usize`（编辑步数）、`relative: f64`（`steps / max(a.len, b.len)`）、`similarity: f64`（`1.0 - relative`）。
- **`damerauLevenshteinSteps(a: string, b: string): usize`**：计算最优字符串对齐编辑步数（每串最多参与前 127 字节）。
- **`damerauLevenshtein(a: string, b: string): DamLevResult`**：返回完整距离与相似度结构体。

## 许可证

MIT
