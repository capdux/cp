[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# home

Zero-heap cross-platform home directory resolver, tilde expand/contract, and XDG config dir builder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:home` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo)) / `npm:untildify` ([https://github.com/sindresorhus/untildify](https://github.com/sindresorhus/untildify)) / `npm:expand-tilde` ([https://github.com/jonschlinkert/expand-tilde](https://github.com/jonschlinkert/expand-tilde)) / `npm:os-homedir` ([https://github.com/sindresorhus/os-homedir](https://github.com/sindresorhus/os-homedir)) / `crates.io:dirs` ([https://github.com/dirs-dev/dirs-rs](https://github.com/dirs-dev/dirs-rs))（原库名 `home` → CP 包名 `home`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `home` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add home
```

```cp
import {
    isSlash,
    writeByte,
    trimTrailingSlashes,
    resolveHomeDir,
    expandTilde,
    contractTilde,
    joinConfigDir,
} from "home";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSlash` | `(ch: u8): bool` | 执行 `isSlash` 条件判定，返回 `bool`。 |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `writeByte` 并返回 `usize`。 |
| `trimTrailingSlashes` | `(s: string): usize` | 调用 `trimTrailingSlashes` 并返回 `usize`。 |
| `resolveHomeDir` | `(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize` | 调用 `resolveHomeDir` 并返回 `usize`。 |
| `expandTilde` | `(path: string, home_dir: string, out: []u8): usize` | 调用 `expandTilde` 并返回 `usize`。 |
| `contractTilde` | `(path: string, home_dir: string, out: []u8): usize` | 调用 `contractTilde` 并返回 `usize`。 |
| `joinConfigDir` | `(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize` | 调用 `joinConfigDir` 并返回 `usize`。 |

## 最小调用示例

```cp
import { trimTrailingSlashes } from "home";

pub fn main() {
    let result = trimTrailingSlashes("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
