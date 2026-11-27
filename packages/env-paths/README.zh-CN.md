[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# env-paths

Zero-allocation cross-platform application directory resolver (Linux XDG, macOS Library, Windows AppData) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:env-paths` ([https://github.com/sindresorhus/env-paths](https://github.com/sindresorhus/env-paths), `MIT`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `env-paths` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add env-paths
```

```cp
import {
    EnvPlatform,
    EnvPathsInput,
    evpTrimTrailingSep,
    evpAppend,
    evpAppendByte,
    evpAppendAppName,
    evpJoinUnix2,
    evpJoinUnix3,
} from "env-paths";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `EnvPlatform` | `Linux, MacOS, Windows` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EnvPathsInput` | `class` | 字段: `platform: EnvPlatform, home: string, xdg_data_home: string, xdg_config_home: string, xdg_cache_home: string, xdg_state_home: string, +4 项` · 方法: `new, linux, macos, windows` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `evpTrimTrailingSep` | `(s: string, sep: u8): string` | 调用 `evpTrimTrailingSep` 并返回 `string`。 |
| `evpAppend` | `(out: []u8, pos: usize, s: string): usize` | 调用 `evpAppend` 并返回 `usize`。 |
| `evpAppendByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `evpAppendByte` 并返回 `usize`。 |
| `evpAppendAppName` | `(out: []u8, pos: usize, name: string, suffix: string): usize` | 调用 `evpAppendAppName` 并返回 `usize`。 |
| `evpJoinUnix2` | `(out: []u8, base: string, name: string, suffix: string): usize` | 调用 `evpJoinUnix2` 并返回 `usize`。 |
| `evpJoinUnix3` | `(out: []u8, base: string, mid: string, name: string, suffix: string): usize` | 调用 `evpJoinUnix3` 并返回 `usize`。 |
| `evpWinAppBase` | `(out: []u8, explicit: string, home: string, fallback_sub: string): usize` | 调用 `evpWinAppBase` 并返回 `usize`。 |
| `evpWinDir` | `(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize` | 调用 `evpWinDir` 并返回 `usize`。 |
| `envDataDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | 调用 `envDataDir` 并返回 `usize`。 |
| `envConfigDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | 调用 `envConfigDir` 并返回 `usize`。 |
| `envCacheDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | 调用 `envCacheDir` 并返回 `usize`。 |
| `envLogDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | 调用 `envLogDir` 并返回 `usize`。 |
| `envTempDir` | `(name: string, inp: EnvPathsInput, out: []u8): usize` | 调用 `envTempDir` 并返回 `usize`。 |

## 最小调用示例

```cp
import { evpTrimTrailingSep } from "env-paths";

pub fn main() {
    let result = evpTrimTrailingSep("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
