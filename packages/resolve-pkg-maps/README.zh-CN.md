[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# resolve-pkg-maps

Zero-heap package.json exports subpath wildcard and condition resolver

| 元信息 | 值 |
| --- | --- |
| 包名 | `resolve-pkg-maps` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add resolve-pkg-maps
```

```cp
import {
    ExportRule,
    rpmEq,
    rpmFindStar,
    rpmHasParentTraversal,
    rpmConditionRank,
    matchSubpathPattern,
    resolveExportTarget,
} from "resolve-pkg-maps";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExportRule` | `class` | 字段: `pattern: string, condition: string, target: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rpmEq` | `(a: string, b: string): bool` | 调用 `rpmEq` 并返回 `bool`。 |
| `rpmFindStar` | `(s: string): usize` | 调用 `rpmFindStar` 并返回 `usize`。 |
| `rpmHasParentTraversal` | `(path: string): bool` | 调用 `rpmHasParentTraversal` 并返回 `bool`。 |
| `rpmConditionRank` | `(cond: string, conditions: &[]string): usize` | 调用 `rpmConditionRank` 并返回 `usize`。 |
| `matchSubpathPattern` | `(pattern: string, subpath: string, matched_star: []u8): i64` | 调用 `matchSubpathPattern` 并返回 `i64`。 |
| `resolveExportTarget` | `(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize` | 调用 `resolveExportTarget` 并返回 `usize`。 |

## 最小调用示例

```cp
import { rpmEq } from "resolve-pkg-maps";

pub fn main() {
    let result = rpmEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
