[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# test-exclude

Zero-heap test and coverage file path glob filter engine

| 元信息 | 值 |
| --- | --- |
| 包名 | `test-exclude` |
| 版本 | `7.0.1` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add test-exclude
```

```cp
import {
    ExcludeConfig,
    texcHasPrefix,
    texcHasSuffix,
    texcContainsSegment,
    texcMatchAt,
    texcMatchRaw,
    matchSimpleGlob,
    texcMatchesExt,
} from "test-exclude";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExcludeConfig` | `class` | 字段: `includes: &[]string, excludes: &[]string, extensions: &[]string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `texcHasPrefix` | `(s: string, prefix: string): bool` | 调用 `texcHasPrefix` 并返回 `bool`。 |
| `texcHasSuffix` | `(s: string, suffix: string): bool` | 调用 `texcHasSuffix` 并返回 `bool`。 |
| `texcContainsSegment` | `(path: string, seg: string): bool` | 调用 `texcContainsSegment` 并返回 `bool`。 |
| `texcMatchAt` | `(pat: string, pi: usize, text: string, ti: usize): bool` | 调用 `texcMatchAt` 并返回 `bool`。 |
| `texcMatchRaw` | `(pat: string, path: string): bool` | 调用 `texcMatchRaw` 并返回 `bool`。 |
| `matchSimpleGlob` | `(pattern: string, path: string): bool` | 调用 `matchSimpleGlob` 并返回 `bool`。 |
| `texcMatchesExt` | `(path: string, ext: string): bool` | 调用 `texcMatchesExt` 并返回 `bool`。 |
| `shouldInstrument` | `(rel_path: string, cfg: ExcludeConfig): bool` | 调用 `shouldInstrument` 并返回 `bool`。 |

## 最小调用示例

```cp
import { texcHasPrefix } from "test-exclude";

pub fn main() {
    let result = texcHasPrefix("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
