[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-key

纯 CP 实现的零堆分配跨平台 `PATH` 环境变量键名查找、值提取以及 `PATH` 列表切分/拼接库，对标 `npm` `path-key`。

- **源包对齐**：对标 `npm` `path-key`（MIT，sindresorhus/path-key）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，切分结果以零拷贝 `PathSpan` 返回，拼接结果写入调用方 `[]u8` 缓冲区。

## 安装与导入

```bash
cap add path-key
```

```cp
import {
    PathSpan,
    EnvPair,
    PathKeyOptions,
    isWindowsPlatform,
    isPathKeyName,
    findPathKeyIndex,
    defaultPathKey,
    pathDelimiter,
    pathKey,
    pathKeyFromPairs,
    findPathValue,
    splitPathEntries,
    joinPathEntries,
    prependPathEntry,
} from "path-key";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `PathSpan` | `class PathSpan { start: usize, len: usize }` | 指向原始 `PATH` 字符串内条目的零拷贝字节区间。 |
| `EnvPair` | `class EnvPair { key: string, value: string }` | 环境变量键值对结构。 |
| `PathKeyOptions` | `class PathKeyOptions { platform: string }` | 平台选项，提供 `PathKeyOptions.posix()` 与 `PathKeyOptions.win32()`。 |
| `isWindowsPlatform` | `fn isWindowsPlatform(platform: string): bool` | 判断平台字符串是否为 Windows（如 `"win32"`、`"windows"`）。 |
| `isPathKeyName` | `fn isPathKeyName(key: string): bool` | 忽略大小写判断是否为 `"PATH"`（匹配 `"Path"`、`"path"`、`"PATH"`）。 |
| `findPathKeyIndex` | `fn findPathKeyIndex(keys: []string, is_windows: bool): i64` | 查找匹配的 `PATH` 键索引（未找到返回 `-1`）；Windows 忽略大小写，POSIX 精确匹配 `"PATH"`。 |
| `defaultPathKey` | `fn defaultPathKey(is_windows: bool): string` | Windows 返回 `"Path"`，POSIX 返回 `"PATH"`。 |
| `pathDelimiter` | `fn pathDelimiter(is_windows: bool): u8` | Windows 返回 `';'`，POSIX 返回 `':'`。 |
| `pathKey` | `fn pathKey(keys: []string, platform: string): string` | 从 `keys` 中解析对应平台的 `PATH` 键名，未找到则回退到 `defaultPathKey`。 |
| `pathKeyFromPairs` | `fn pathKeyFromPairs(env: []EnvPair, platform: string): string` | 从 `EnvPair` 列表中解析对应平台的 `PATH` 键名。 |
| `findPathValue` | `fn findPathValue(env: []EnvPair, platform: string): string?` | 查找并返回对应平台的 `PATH` 值，不存在返回 `null`。 |
| `splitPathEntries` | `fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize` | 按平台分隔符切分 `path_val` 写入 `out`，返回写入条数。 |
| `joinPathEntries` | `fn joinPathEntries(entries: []string, is_windows: bool, out: []u8): usize` | 按平台分隔符拼接 `entries` 写入 `out`，返回写入字节数。 |
| `prependPathEntry` | `fn prependPathEntry(entry: string, existing_path: string, is_windows: bool, out: []u8): usize` | 将 `entry` 前置到 `existing_path` 并写入 `out`，返回写入字节数。 |

## 使用示例

### 1. 跨平台查找 `PATH` 键名与值

```cp
import { EnvPair, pathKeyFromPairs, findPathValue } from "path-key";

fn lookupDemo(): void {
    const env: [2]EnvPair = [
        EnvPair { key: "SystemRoot", value: "C:\\Windows" },
        EnvPair { key: "Path", value: "C:\\bin;C:\\tools" },
    ];
    const key = pathKeyFromPairs(env[0..], "win32"); // "Path"
    const val = findPathValue(env[0..], "win32");    // Some("C:\\bin;C:\\tools")
}
```

### 2. 零拷贝切分与栈缓冲前置路径

```cp
import { PathSpan, splitPathEntries, prependPathEntry } from "path-key";

fn manipulatePath(): void {
    const raw = "/usr/local/bin:/usr/bin";
    let mut spans: [8]PathSpan = [PathSpan { start: 0, len: 0 }; 8];
    const count = splitPathEntries(raw, false, spans[0..]); // 2

    let mut out: [128]u8 = [0; 128];
    const n = prependPathEntry("/opt/cap/bin", raw, false, out[0..]);
    // out[0..n] == "/opt/cap/bin:/usr/local/bin:/usr/bin"
}
```

## 与源包差异与安全亮点

- **显式传参设计**：显式传入 `keys` / `env` 与 `platform`，不依赖全局 `process.env`，适合多平台构建工具与交叉编译场景。
- **零分配路径工具集**：在兼容 npm `path-key` 的基础上，新增 `splitPathEntries`、`joinPathEntries` 与 `prependPathEntry`，全程零堆分配。
