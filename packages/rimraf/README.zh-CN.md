[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rimraf

Zero-allocation recursive deletion safety guard, root-path validator, and bottom-up delete planner for CP, reusing fast-glob.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rimraf` ([https://github.com/isaacs/rimraf](https://github.com/isaacs/rimraf))（原库名 `rimraf` → CP 包名 `rimraf`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rimraf` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rimraf
```

```cp
import {
    RimrafOptions,
    RimrafPlan,
    rmfIsAsciiAlpha,
    rmfIsSep,
    rmfHasNul,
    rmfPathDepth,
    isRootPath,
    validateRimrafTarget,
} from "rimraf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RimrafOptions` | `class` | 字段: `preserve_root: bool, max_retries: u8, retry_delay_ms: u32, is_windows: bool` · 方法: `defaults` |
| `RimrafPlan` | `class` | 字段: `valid: bool, paths: [32]string, count: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rmfIsAsciiAlpha` | `(b: u8): bool` | 调用 `rmfIsAsciiAlpha` 并返回 `bool`。 |
| `rmfIsSep` | `(b: u8, is_windows: bool): bool` | 调用 `rmfIsSep` 并返回 `bool`。 |
| `rmfHasNul` | `(s: string): bool` | 调用 `rmfHasNul` 并返回 `bool`。 |
| `rmfPathDepth` | `(path: string, is_windows: bool): usize` | 调用 `rmfPathDepth` 并返回 `usize`。 |
| `isRootPath` | `(path: string, is_windows: bool): bool` | 执行 `isRootPath` 条件判定，返回 `bool`。 |
| `validateRimrafTarget` | `(target: string, opts: RimrafOptions): bool` | 校验输入合法性，返回 `bool`。 |
| `rmfSortByDepthDesc` | `(paths: []string, count: usize, is_windows: bool)` | 就地执行 `rmfSortByDepthDesc` 操作。 |
| `planBottomUpDelete` | `(entries: []string, opts: RimrafOptions): RimrafPlan` | 调用 `planBottomUpDelete` 并返回 `RimrafPlan`。 |
| `planFilteredDelete` | `(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan` | 调用 `planFilteredDelete` 并返回 `RimrafPlan`。 |

## 最小调用示例

```cp
import { isRootPath } from "rimraf";

pub fn main() {
    let result = isRootPath("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
