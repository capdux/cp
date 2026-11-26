# @cap/strsim

零堆分配的 CP 7 合 1 字符串距离与相似度度量引擎 — 洁净室 MIT 实现，对标 Rust crates.io #46 `strsim`（13.25 亿+ 总下载）。

Zero-allocation 7-in-1 string distance & similarity metrics engine for CP — clean-room MIT parity with Rust `strsim` (crates.io #46, 1.325B+ downloads).

## 安装 / Install

```bash
cap add @cap/strsim
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `hamming` | `(a: string, b: string) -> usize?` | 汉明距离（不等长返回 `null`） |
| `levenshtein` | `(a: string, b: string) -> usize` | Levenshtein 编辑距离 |
| `normalizedLevenshtein` | `(a: string, b: string) -> f64` | 归一化 Levenshtein 相似度 `[0.0, 1.0]` |
| `osaDistance` | `(a: string, b: string) -> usize` | Optimal String Alignment 相邻换位编辑距离 |
| `jaro` | `(a: string, b: string) -> f64` | Jaro 相似度 `[0.0, 1.0]` |
| `jaroWinkler` | `(a: string, b: string) -> f64` | Jaro-Winkler 相似度（前缀加权 `p = 0.1`，`jaro > 0.7`） |
| `sorensenDice` | `(a: string, b: string) -> f64` | 基于 2-gram 交集的 Sørensen-Dice 系数 `[0.0, 1.0]` |

## 使用示例 / Usage

```cp
import {
    hamming,
    levenshtein,
    normalizedLevenshtein,
    osaDistance,
    jaro,
    jaroWinkler,
    sorensenDice,
} from "@cap/strsim";

const d_ham = hamming("karolin", "kathrin").?;    // 3
const d_lev = levenshtein("kitten", "sitting");   // 3
const d_osa = osaDistance("ca", "ac");            // 1
const s_jw  = jaroWinkler("MARTHA", "MARHTA");    // ~0.9611
const s_sd  = sorensenDice("night", "nacht");     // 0.25
```

## License

MIT
