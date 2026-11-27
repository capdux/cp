[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# camino

> **源包溯源**：吸收自 **crates.io** [`camino`](https://github.com/camino-rs/camino)（v1 / `MIT OR Apache-2.0`），CP 官方洁净室实现 `camino@1.0.0`。

纯 CP 实现的零堆分配、严格 UTF-8 校验路径引擎（`Utf8PathBuf`）。底层复用 `utf8parse` 对原始字节输入执行严格 UTF-8 合法性校验。

## 核心亮点

- **零堆分配内联路径**：`Utf8PathBuf` 内联存储最长 512 字节路径（`CAMINO_MAX_PATH = 512`），无需任何堆内存分配。
- **零栈清零优化**：`fromStr`、`fromBytes`、`parent`、`stripPrefix` 均使用 `undefined` 初始化底层 `buf`，消除每次路径操作 512 字节的栈清零开销。
- **严格 UTF-8 安全保证**：`Utf8PathBuf.fromBytes(b)` 自动校验 UTF-8 合法性，非法字节流直接返回 `null`。
- **完整 Rust `camino` 体验**：支持 `push`、`pop`、`parent`、`fileNameInto`、`fileStemInto`、`extensionInto`、`setFileName`、`setExtension`、`componentCount`、`hasPrefixPath` 与 `stripPrefix`。

## 零成本迁移（Rust → CP）

直接沿用原包名 `camino`，无需改名：

**Rust：**
```rust
use camino::Utf8PathBuf;
let mut p = Utf8PathBuf::from("/usr/local");
p.push("bin/cap");
```

**CP：**
```typescript
import { Utf8PathBuf, CAMINO_MAX_PATH } from "camino";
```

## 实战示例

### 1. 路径拼接、扩展名修改与前缀剥离

```typescript
import { Utf8PathBuf } from "camino";

export fn demoPath(): bool {
    let mut p = Utf8PathBuf.fromStr("/var/log");
    p.push("app/server.log");
    // p == "/var/log/app/server.log"

    let mut ext_buf: [32]u8 = undefined;
    let ext_len = p.extensionInto(ext_buf[0..32]); // "log"

    _ = p.setExtension("txt"); // "/var/log/app/server.txt"

    if (let rel = p.stripPrefix("/var/log")) {
        return rel.equals("app/server.txt") && ext_len != null;
    }
    return false;
}
```

### 2. 严格校验原始字节构造 `Utf8PathBuf`

```typescript
import { Utf8PathBuf } from "camino";

export fn fromRawPath(raw: []u8): bool {
    if (let p = Utf8PathBuf.fromBytes(raw)) {
        return p.isAbsolute() && p.componentCount() > 0;
    }
    return false; // 非法 UTF-8 或超过 512 字节上限
}
```

## 核心 API 速览

- **常量**：`CAMINO_MAX_PATH: usize = 512`
- **构造**：`Utf8PathBuf.fromStr(s: string)`、`Utf8PathBuf.fromBytes(b: []u8): Utf8PathBuf?`
- **查询**：`isAbsolute()`、`isRelative()`、`len()`、`isEmpty()`、`equals(other)`、`componentCount()`、`hasPrefixPath(base)`
- **提取**：`parent(): Utf8PathBuf?`、`stripPrefix(base): Utf8PathBuf?`、`fileNameInto(dst): usize?`、`fileStemInto(dst): usize?`、`extensionInto(dst): usize?`
- **修改**：`push(path)`、`pop(): bool`、`setFileName(name)`、`setExtension(ext): bool`

## 许可证

MIT
