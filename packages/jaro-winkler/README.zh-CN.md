[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jaro-winkler

Zero-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:jaro-winkler` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `jaro-winkler` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jaro-winkler
```

```cp
import { jaro, jaroWinklerWithScale, jaroWinkler, bestMatchIndex } from "jaro-winkler";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jaro` | `(a: string, b: string): f64` | 调用 `jaro` 并返回 `f64`。 |
| `jaroWinklerWithScale` | `(a: string, b: string, prefix_scale: f64): f64` | 调用 `jaroWinklerWithScale` 并返回 `f64`。 |
| `jaroWinkler` | `(a: string, b: string): f64` | 调用 `jaroWinkler` 并返回 `f64`。 |
| `bestMatchIndex` | `(query: string, candidates: &[]string, min_score: f64): i64` | 调用 `bestMatchIndex` 并返回 `i64`。 |

## 最小调用示例

```cp
import { jaro } from "jaro-winkler";

pub fn main() {
    let result = jaro("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
