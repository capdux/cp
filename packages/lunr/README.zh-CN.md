[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lunr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation in-memory inverted index and BM25 full-text search engine with prefix matching in pure CP

## 安装与导入

```bash
cap add lunr
```

```cp
import { fnv1aLower } from "lunr";
```

## 核心导出 API

- `export fn fnv1aLower(data: &[]u8, start: usize, end: usize): u64;`
- `export fn lnApprox(x_in: f64): f64;`
- `export fn bm25Idf(n_docs: usize, df: u16): f64;`
- `export fn bm25TermScore(idf: f64, tf: u16, dl: u32, avgdl: f64): f64;`
- `export fn sortHitsDesc(hits: []SearchHit, count: usize);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lunr@1.0.0`)，许可证：MIT
