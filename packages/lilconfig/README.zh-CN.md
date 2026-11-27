[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lilconfig

Zero-heap pure CP lightweight configuration explorer with JSON/JS defaults and custom loader registry (clean-room MIT implementation of npm lilconfig v3)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lilconfig` ([https://github.com/antonk52/lilconfig](https://github.com/antonk52/lilconfig))

| 元信息 | 值 |
| --- | --- |
| 包名 | `lilconfig` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lilconfig
```

```cp
import {
    LilconfigOptions,
    CustomLoaderEntry,
    LilExplorer,
    lilEndsWithIgnoreCase,
    lilBasename,
    lilIsNoExt,
    formatLilSearchPlace,
    isSupportedByDefault,
} from "lilconfig";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LilconfigOptions` | `class` | 字段: `module_name: string, stop_dir: string, package_prop: string, ignore_empty_search_places: bool` · 方法: `defaults` |
| `CustomLoaderEntry` | `class` | 字段: `ext: string, loader_id: u8` |
| `LilExplorer` | `class` | 字段: `opts: LilconfigOptions, planner: SearchPlanner, custom_loaders: [8]CustomLoaderEntry, custom_count: usize` · 方法: `new, registerLoader, resolveLoader, nextSearchDir, candidatePath, makeResult` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lilEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | 调用 `lilEndsWithIgnoreCase` 并返回 `bool`。 |
| `lilBasename` | `(filepath: string): string` | 调用 `lilBasename` 并返回 `string`。 |
| `lilIsNoExt` | `(filepath: string): bool` | 调用 `lilIsNoExt` 并返回 `bool`。 |
| `formatLilSearchPlace` | `(module_name: string, idx: usize, out: []u8): usize` | 通过 `formatLilSearchPlace` 编码或格式化输出，返回 `usize`。 |
| `isSupportedByDefault` | `(filepath: string): bool` | 执行 `isSupportedByDefault` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { formatLilSearchPlace } from "lilconfig";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLilSearchPlace("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
