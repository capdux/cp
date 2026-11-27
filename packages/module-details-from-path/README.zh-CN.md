[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# module-details-from-path

Zero-heap node_modules and cap_modules path parser extracting package name, basedir, and subpath

| 元信息 | 值 |
| --- | --- |
| 包名 | `module-details-from-path` |
| 版本 | `1.0.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add module-details-from-path
```

```cp
import {
    ModuleDetails,
    mdfpIsSep,
    mdfpMatchDir,
    mdfpSkipSeps,
    mdfpFindSep,
    mdfpEmpty,
    parseModulePath,
} from "module-details-from-path";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ModuleDetails` | `class` | 字段: `name: string, basedir: string, subpath: string, found: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mdfpIsSep` | `(ch: u8): bool` | 调用 `mdfpIsSep` 并返回 `bool`。 |
| `mdfpMatchDir` | `(s: string, pos: usize, dir: string): bool` | 调用 `mdfpMatchDir` 并返回 `bool`。 |
| `mdfpSkipSeps` | `(s: string, start: usize): usize` | 调用 `mdfpSkipSeps` 并返回 `usize`。 |
| `mdfpFindSep` | `(s: string, start: usize): usize` | 调用 `mdfpFindSep` 并返回 `usize`。 |
| `mdfpEmpty` | `(): ModuleDetails` | 调用 `mdfpEmpty` 并返回 `ModuleDetails`。 |
| `parseModulePath` | `(file_path: string): ModuleDetails` | 通过 `parseModulePath` 解析输入数据，返回 `ModuleDetails`。 |

## 最小调用示例

```cp
import { parseModulePath } from "module-details-from-path";

pub fn main() {
    let result = parseModulePath("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
