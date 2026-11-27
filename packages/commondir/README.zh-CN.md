[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# commondir

纯 CP 实现的零堆分配公共父目录计算库，用于求解一组文件路径的最深公共父目录，并提供 POSIX 路径规范化与相对路径解析能力。

- **包名**：`commondir`（`v1.0.1`）
- **上游对齐**：洁净室移植自 `npm/commondir`（`v1.0.1`，`MIT`，`substack/node-commondir`），同时支持 `commondir(files)` 与 `commondir(basedir, relfiles)` 两种用法。

## 安装与导入

```bash
cap add commondir
```

```typescript
import {
    COMMONDIR_MAX_PATH,
    normalizePosixPath, resolvePosixPath,
    commonDirTwo, commonDir,
    commonDirTwoNormalized, commonDirNormalized, commonDirWithBase,
} from "commondir";
```

## 核心 API

| 符号 | 签名 / 值 | 说明 |
| --- | --- | --- |
| `COMMONDIR_MAX_PATH` | `1024`（`usize`） | 内部路径规范化缓冲区最大字节数。 |
| `normalizePosixPath` | `(path: string, out: []u8): usize` | 折叠重复斜杠、`.` 与 `..` 段并写入 `out`，返回写入字节数。 |
| `resolvePosixPath` | `(base: string, rel: string, out: []u8): usize` | 基于 `base` 解析相对路径 `rel`（绝对路径直接规范化）并写入 `out`。 |
| `commonDirTwo` | `(a: string, b: string): string` | 零拷贝求两条已规范化路径的最深公共父目录（借用自 `a`）。 |
| `commonDir` | `(paths: &[]string): string` | 零拷贝求路径切片的最深公共父目录（借用自 `paths[0]`）。 |
| `commonDirTwoNormalized` | `(a: string, b: string, out: []u8): usize` | 先规范化两条路径，再将公共父目录写入 `out`。 |
| `commonDirNormalized` | `(paths: &[]string, out: []u8): usize` | 先规范化全部路径，再将最深公共父目录写入 `out`。 |
| `commonDirWithBase` | `(base: string, files: &[]string, out: []u8): usize` | 将 `files` 相对 `base` 解析后求最深公共目录写入 `out`（对应 `commondir(basedir, relfiles)`）。 |

## 使用示例

### 1. 已规范化路径的零拷贝公共目录计算

```typescript
import { commonDir, commonDirTwo } from "commondir";

fn main() {
    let paths: [3]string = [
        "/x/y/z/a.txt",
        "/x/y/w/b.txt",
        "/x/y/z/c.txt",
    ];
    let dir = commonDir(paths[0..]);             // "/x/y"
    let two = commonDirTwo("/a/b/c", "/a/b/d");  // "/a/b"
}
```

### 2. 含 `..` 与重复斜杠的路径规范化求解

```typescript
import { commonDirNormalized } from "commondir";

fn main() {
    let paths: [2]string = [
        "/a/b/../c/x.cp",
        "/a//c/./y.cp",
    ];
    let out: [256]u8 = undefined;
    let n = commonDirNormalized(paths[0..], out[0..]);
    // out[0..n] == "/a/c"
}
```

### 3. 基于基准目录解析相对路径（`commondir(base, files)`）

```typescript
import { commonDirWithBase } from "commondir";

fn main() {
    let rels: [2]string = ["../c/foo.cp", "../c/bar/baz.cp"];
    let out: [256]u8 = undefined;
    let n = commonDirWithBase("/a/b", rels[0..], out[0..]);
    // out[0..n] == "/a/c"
}
```

## 与源包差异及零拷贝安全亮点

- **零堆分配**：`commonDir` / `commonDirTwo` 直接返回输入字符串的零拷贝子切片；规范化版本写入调用方提供的 `out: []u8` 栈缓冲区。
- **显式区分快速路径与规范化路径**：已清洁路径走零拷贝 `commonDir`，含 `.` / `..` / `//` 或相对基准目录时调用 `commonDirNormalized` / `commonDirWithBase`。
- **路径段边界安全**：按路径分隔符对齐比较，绝不会把 `/foo` 与 `/foobar` 误判为公共前缀，并正确保留根目录 `"/"`。
