[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# toml_edit

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`crates.io:toml_edit`](https://github.com/toml-rs/toml)（MIT，v0.22 语义）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 TOML v1.0 文档解析、类型化读写修改与规范分节格式化引擎。

## 零成本迁移对照（Rust → CP）

直接沿用原包名 `"toml_edit"`，无需改名即可完成迁移：

| Rust (`crates.io:toml_edit`) | CP (`toml_edit`) |
| :--- | :--- |
| `use toml_edit::DocumentMut;` | `import { TomlDoc } from "toml_edit";` |
| `let mut doc = src.parse::<DocumentMut>()?;` | `let doc = TomlDoc.parse(src);` |
| `doc["package"]["name"].as_str()` | `doc.getStr("package", "name")` |
| `doc["server"]["port"] = value(8080);` | `doc.setInt("server", "port", 8080)` |
| `doc.to_string()` | `doc.format(out)` |

## 核心 API

### 导出常量

- `TOML_STR: u8 = 1`
- `TOML_INT: u8 = 2`
- `TOML_BOOL: u8 = 3`
- `TOML_MAX_ENTRIES: usize = 64`

### 类型与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `TomlEntry { section, key, kind, str_val, int_val, bool_val }` | 带类型标签的条目结构（`TomlEntry.empty()`）。 |
| `TomlDoc.new(): TomlDoc` | 创建空 64 槽位栈上文档。 |
| `TomlDoc.parse(src: string): TomlDoc` | 解析 TOML 文本（顶层键、`[table]` 表头、`#` 注释、字符串/整数/布尔）。 |
| `doc.len(): usize` / `doc.hasKey(section, key): bool` | 查询条目数或键是否存在。 |
| `doc.getStr(section, key): string?` | 读取字符串值（缺失或类型不匹配返回 `null`）。 |
| `doc.getInt(section, key): i64?` | 读取整数值（缺失或类型不匹配返回 `null`）。 |
| `doc.getBool(section, key): bool?` | 读取布尔值（缺失或类型不匹配返回 `null`）。 |
| `doc.setStr(*_, section, key, val: string): bool` | 写入或覆盖字符串值（满 64 条返回 `false`）。 |
| `doc.setInt(*_, section, key, val: i64): bool` | 写入或覆盖 `i64` 值。 |
| `doc.setBool(*_, section, key, val: bool): bool` | 写入或覆盖 `bool` 值。 |
| `doc.format(out: []u8): usize` | 将文档按顶层键 + 分组 `[section]` 规范格式化写入 `out`。 |

## 实战示例

### 解析、修改并重新格式化

```cp
import { TomlDoc } from "toml_edit";

fn demoEdit() {
    const src = "title = \"cp-app\"\n[server]\nhost = \"127.0.0.1\"\nport = 8080\ntls = false\n";
    let doc = TomlDoc.parse(src);

    const title = doc.getStr("", "title");     // "cp-app"
    const port = doc.getInt("server", "port"); // 8080

    // 原地更新并新增分节键
    * = doc.setInt("server", "port", 9443);
    * = doc.setBool("server", "tls", true);
    * = doc.setStr("metrics", "bind", "0.0.0.0");

    let out: [512]u8 = undefined;
    const n = doc.format(out[0..]);
    * = title; * = port; * = n;
}
```

### 类型安全查询与下划线数字

```cp
import { TomlDoc } from "toml_edit";

fn demoTyped() {
    const src = "[limits]\nmax_bytes = 1_048_576\nstrict = true\n";
    const doc = TomlDoc.parse(src);

    // 支持 TOML 下划线分隔整数
    const max_b = doc.getInt("limits", "max_bytes"); // 1048576
    // 类型不匹配安全返回 null
    const wrong = doc.getStr("limits", "max_bytes"); // null
    * = max_b; * = wrong;
}
```

## 性能与安全亮点

- **零堆分配**：`TomlDoc` 在栈上内联 `[64]TomlEntry`，`format()` 使用栈上 `[64]bool = undefined` 位图完成分节聚合，无需哈希表。
- **`@memcpy` 高速序列化**：字符串转义与条目写入均通过 `@memcpy` 直写 `out: []u8` 并内置越界保护。
- **类型安全**：`getStr`/`getInt`/`getBool` 严格检查 `kind` 并返回可选类型 `T?`，杜绝隐式类型混淆。

## 许可协议

MIT
