[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# npm-run-path

Zero-heap PATH builder that prepends ancestor node_modules/.bin (and cap_modules/.bin) directories for local binary execution (npm npm-run-path equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:npm-run-path` ([https://github.com/sindresorhus/npm-run-path](https://github.com/sindresorhus/npm-run-path))

| 元信息 | 值 |
| --- | --- |
| 包名 | `npm-run-path` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add npm-run-path
```

```cp
import {
    RunPathOptions,
    nrpPushBytes,
    nrpPushChar,
    nrpIsSep,
    dirnameOf,
    countAncestorDirs,
    buildRunPath,
    pathKey,
} from "npm-run-path";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RunPathOptions` | `class` | 字段: `cwd: string, path_env: string, exec_path: string, prefer_local: bool, add_exec_path: bool, is_windows: bool, +1 项` · 方法: `defaults, forCap` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nrpPushBytes` | `(out: []u8, pos: usize, s: string): usize` | 调用 `nrpPushBytes` 并返回 `usize`。 |
| `nrpPushChar` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `nrpPushChar` 并返回 `usize`。 |
| `nrpIsSep` | `(c: u8, is_windows: bool): bool` | 调用 `nrpIsSep` 并返回 `bool`。 |
| `dirnameOf` | `(path: string, is_windows: bool): string` | 调用 `dirnameOf` 并返回 `string`。 |
| `countAncestorDirs` | `(cwd: string, is_windows: bool): usize` | 调用 `countAncestorDirs` 并返回 `usize`。 |
| `buildRunPath` | `(opts: RunPathOptions, out: []u8): usize` | 调用 `buildRunPath` 并返回 `usize`。 |
| `pathKey` | `(is_windows: bool): string` | 调用 `pathKey` 并返回 `string`。 |

## 最小调用示例

```cp
import { nrpPushBytes } from "npm-run-path";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nrpPushBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
