[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pathdiff

面向 CP 语言的零堆分配相对路径计算库，用于求解目标路径相对于基准目录的相对路径。

对齐 Rust crates.io 源包 [`pathdiff`](https://github.com/Manishearth/pathdiff)（MIT OR Apache-2.0）。

## 安装与导入

```bash
cap add pathdiff
```

```cp
import {
    PATHDIFF_MAX_SEGS,
    hasSameRoot,
    normalizePath,
    diffPaths,
    diffUtf8Paths,
} from "pathdiff";
```

## 核心 API

### 常量

| 符号 | 类型 | 说明 |
| --- | --- | --- |
| `PATHDIFF_MAX_SEGS` | `usize = 64` | 栈上路径归一化缓冲区支持的最大路径分段数。 |

### 函数列表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `hasSameRoot` | `(path: string, base: string): bool` | 检查 `path` 与 `base` 是否拥有相同根类型（同为绝对路径或同为相对路径）。 |
| `normalizePath` | `(path: string, out: []u8): usize?` | 对路径执行词法级归一化（消解 `.` 和 `..`）并写入 `out`，缓冲区不足时返回 `null`。 |
| `diffPaths` | `(target: string, base: string, out: []u8): usize?` | 计算从 `base` 到 `target` 的相对路径写入 `out`；根不兼容或容量不足返回 `null`。 |
| `diffUtf8Paths` | `(path: string, base: string, out: []u8): usize?` | `diffPaths` 的 UTF-8 别名（对齐 Rust `diff_utf8_paths`）。 |

## 使用示例

### 1. 计算两个目录之间的相对路径

```cp
import { print } from "cap:io";
import { diffPaths } from "pathdiff";

pub fn main() {
    let buf: [128]u8 = undefined;
    let n = diffPaths("/usr/local/bin/cp", "/usr/share/doc", buf[0..128]);
    if (n != null) {
        print("relative: {}\n", buf[0..n!]); // "../../local/bin/cp"
    }
}
```

### 2. 词法路径归一化

```cp
import { print } from "cap:io";
import { normalizePath } from "pathdiff";

pub fn main() {
    let buf: [128]u8 = undefined;
    let n = normalizePath("/a/b/../c/./d", buf[0..128]);
    if (n != null) {
        print("normalized: {}\n", buf[0..n!]); // "/a/c/d"
    }
}
```

### 3. 根路径兼容性校验

```cp
import { print } from "cap:io";
import { hasSameRoot, diffUtf8Paths } from "pathdiff";

pub fn main() {
    if (hasSameRoot("/foo/bar", "baz/qux")) {
        let out: [64]u8 = undefined;
        let _ = diffUtf8Paths("/foo/bar", "baz/qux", out[0..64]);
    } else {
        print("绝对路径与相对路径无法直接求差\n");
    }
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配输出**：直接向调用方提供的 `out: []u8` 缓冲区写入结果，无需分配 `PathBuf`。
- **可选类型安全返回**：根路径不匹配或输出缓冲区溢出时统一返回 `null`（`usize?`），避免静默截断或崩溃。
- **纯词法无系统调用**：所有解析在固定栈数组（上限 `PATHDIFF_MAX_SEGS = 64` 段）内完成，不访问文件系统。
