[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ini

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:ini`](https://github.com/npm/ini)（ISC）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 INI / `.gitconfig` / `.npmrc` / `.editorconfig` 解析与序列化库。支持 `[section]` 分节、`#`/`;` 注释、引号值、布尔/整数类型化读取以及带自动加引号的缓冲区写入器。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"ini"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:ini`) | CP (`ini`) |
| :--- | :--- |
| `import ini from "ini";` | `import { IniDoc, IniWriter } from "ini";` |
| `const cfg = ini.parse(text);` | `const doc = IniDoc.parse(text);` |
| `cfg.server.port` | `doc.getInt(text, "server", "port", 8080)` |
| `cfg.server.tls` | `doc.getBool(text, "server", "tls", false)` |
| `ini.stringify(obj)` | `IniWriter.init(buf)` + `writeSection` / `writeEntry` |

## 核心 API

### 导出常量

- `MAX_ENTRIES: usize = 128`

### 类型与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `IniEntry { section_start, section_len, key_start, key_len, val_start, val_len }` | 基于偏移量的零拷贝索引项，不复制原始字符串。 |
| `IniDoc.parse(src: &[]u8): IniDoc` | 解析 INI 字节流，最多索引 128 条键值项。 |
| `doc.len(): usize` | 返回已解析的条目总数。 |
| `doc.findIndex(src: &[]u8, section: &[]u8, key: &[]u8): i64` | 查找条目索引（`>= 0`），不存在返回 `-1`。 |
| `doc.has(src: &[]u8, section: &[]u8, key: &[]u8): bool` | 判断 `(section, key)` 是否存在。 |
| `doc.get(src: &[]u8, section: &[]u8, key: &[]u8): &[]u8` | 返回借用的值切片（不存在时返回空切片）。 |
| `doc.getBool(src, section, key, default_val: bool): bool` | 解析布尔值（支持 `true`/`false`/`yes`/`no`/`on`/`off`/`1`/`0`）。 |
| `doc.getInt(src, section, key, default_val: i64): i64` | 解析十进制 `i64`，无效或缺失时返回 `default_val`。 |
| `IniWriter.init(buf: []u8): IniWriter` | 基于调用方 `buf` 创建零堆写入器。 |
| `w.writeSection(*_, name: &[]u8): bool` | 写入 `[name]\n` 分节头；空间不足返回 `false`。 |
| `w.writeEntry(*_, key: &[]u8, value: &[]u8): bool` | 写入 `key = value\n`（含 `;`/`#`/首尾空格时自动双引号包裹）。 |
| `w.written(): &[]u8` / `w.len(): usize` | 获取已写入切片或字节长度。 |

## 实战示例

### 解析与类型化读取

```cp
import { IniDoc } from "ini";

fn demoParse() {
    const src = "; app config\nname = demo\n[server]\nhost = 127.0.0.1\nport = 9000\ntls = true\n";
    const doc = IniDoc.parse(src);

    // 顶层全局键使用空节名 ""
    const name = doc.get(src, "", "name");                 // "demo"
    const port = doc.getInt(src, "server", "port", 8080);  // 9000
    const tls = doc.getBool(src, "server", "tls", false);  // true
    const timeout = doc.getInt(src, "server", "timeout", 30); // 30（默认值）
    * = name; * = port; * = tls; * = timeout;
}
```

### 自动加引号序列化与溢出检测

```cp
import { IniWriter } from "ini";

fn demoWrite() {
    let buf: [256]u8 = undefined;
    let w = IniWriter.init(buf[0..]);

    * = w.writeSection("database");
    * = w.writeEntry("host", "localhost");
    // 值包含 '#' -> 自动使用双引号包裹
    * = w.writeEntry("note", "primary #1");

    if (!w.overflow) {
        const out = w.written();
        * = out;
    }
}
```

## 性能与安全亮点

- **零拷贝偏移索引**：`IniDoc` 使用 `entries: [128]IniEntry = undefined` 仅存起止偏移，无字符串拷贝且初始化零清零。
- **`@memcpy` 高速写入**：`IniWriter` 通过 `@memcpy` 直接写入目标缓冲，内置越界拦截与粘性 `overflow` 标志。
- **借用安全**：`doc.get(src, ...)` 直接返回指向 `src` 的只读借用切片，无悬空指针与堆泄漏。

## 许可协议

MIT
