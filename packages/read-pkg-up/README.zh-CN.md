[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# read-pkg-up

纯 CP 实现的零分配向上查找、解析与规范化 `package.json` / `cap.json` 工具包，组合复用 `find-up` 与 `read-pkg`。

- **源包对齐**：对标 `npm` `read-pkg-up`（MIT，sindresorhus/read-pkg-up）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，基于调用方传入的 `[]PkgFileEntry` 切片与 `[]u8` 路径缓冲区完成解析。

## 安装与导入

```bash
cap add read-pkg-up
```

```cp
import {
    PkgFileEntry,
    ReadPkgUpOptions,
    PkgQuickMeta,
    ReadPkgUpResult,
    parentDir,
    joinPath,
    extractPkgStringField,
    parseQuickPkgMeta,
    readPackageUpNamed,
    readPackageUpInEntries,
    readPackageUpWithOptions,
} from "read-pkg-up";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `PkgFileEntry` | `class PkgFileEntry { path: string, content: string }` | 内存文件条目（路径与 JSON 原文），用于确定性向上查找。 |
| `ReadPkgUpOptions` | `class ReadPkgUpOptions { cwd: string, stop_at: string, normalize: bool, prefer_cap_json: bool }` | 查找选项；`ReadPkgUpOptions.defaults(cwd)` 构造默认配置。 |
| `PkgQuickMeta` | `class PkgQuickMeta { name, version, description: string, has_name, has_version, has_description: bool }` | 从 JSON 原文零拷贝提取的轻量元数据。 |
| `ReadPkgUpResult` | `class ReadPkgUpResult { found: bool, pkg_path_len: usize, pkg: ReadPkgResult }` | 查找结果，含 `found`、`path_out` 写入长度与 `ReadPkgResult`。 |
| `parentDir` | `fn parentDir(path: string): string?` | 返回 `path` 的父目录切片，到达根目录时返回 `null`。 |
| `joinPath` | `fn joinPath(dir: string, name: string, out: []u8): usize` | 将 `dir` 与 `name` 拼接写入 `out`，返回写入字节数。 |
| `extractPkgStringField` | `fn extractPkgStringField(json: string, key: string): string?` | 零分配提取顶层字符串字段值。 |
| `parseQuickPkgMeta` | `fn parseQuickPkgMeta(json: string): PkgQuickMeta` | 单遍零拷贝提取 `name`、`version`、`description`。 |
| `readPackageUpNamed` | `fn readPackageUpNamed(cwd, stop_at, filename: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | 从 `cwd` 向上查找到 `stop_at`，匹配指定 `filename` 并解析。 |
| `readPackageUpInEntries` | `fn readPackageUpInEntries(cwd, stop_at: string, normalize: bool, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | 向上查找并解析 `"package.json"`。 |
| `readPackageUpWithOptions` | `fn readPackageUpWithOptions(opts: ReadPkgUpOptions, entries: []PkgFileEntry, path_out: []u8): ReadPkgUpResult` | 按 `ReadPkgUpOptions` 查找（支持 `prefer_cap_json` 优先查找 `cap.json`）。 |

## 使用示例

### 1. 向上查找并解析最近的 `package.json`

```cp
import { PkgFileEntry, readPackageUpInEntries } from "read-pkg-up";

fn findPkg(): void {
    const entries: [1]PkgFileEntry = [
        PkgFileEntry {
            path: "/repo/package.json",
            content: "{\"name\":\"demo\",\"version\":\"1.0.0\"}",
        },
    ];
    let mut path_buf: [256]u8 = [0; 256];
    const res = readPackageUpInEntries("/repo/src/app", "/repo", true, entries[0..], path_buf[0..]);
    if (res.found) {
        // path_buf[0..res.pkg_path_len] == "/repo/package.json"
    }
}
```

### 2. 使用 `ReadPkgUpOptions` 优先解析 `cap.json`

```cp
import { PkgFileEntry, ReadPkgUpOptions, readPackageUpWithOptions } from "read-pkg-up";

fn findCapManifest(entries: []PkgFileEntry): void {
    let mut opts = ReadPkgUpOptions.defaults("/repo/packages/app/src");
    opts.stop_at = "/repo";
    opts.prefer_cap_json = true;
    let mut path_buf: [256]u8 = [0; 256];
    const res = readPackageUpWithOptions(opts, entries, path_buf[0..]);
}
```

### 3. 轻量零拷贝元数据提取

```cp
import { parseQuickPkgMeta, extractPkgStringField } from "read-pkg-up";

fn quickMeta(): void {
    const raw = "{\"name\":\"my-pkg\",\"version\":\"2.1.0\",\"description\":\"Fast\"}";
    const meta = parseQuickPkgMeta(raw);
    const ver = extractPkgStringField(raw, "version"); // Some("2.1.0")
}
```

## 与源包差异与安全亮点

- **纯函数可测**：接收 `[]PkgFileEntry` 列表而非直接访问异步文件系统，适合 Monorepo 虚拟文件树解析与零分配单测。
- **原生支持 `cap.json`**：通过 `ReadPkgUpOptions.prefer_cap_json` 可优先匹配 CP 清单文件。
- **模块化组合**：底层复用 `find-up` 目录遍历与 `read-pkg` 清单解析。
