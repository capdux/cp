# CP 标准库：TOML v1.0.0 配置解析、类型安全访问与序列化手册 (`cap:toml`)

## 一、 架构总览与设计准则

`cap:toml` 是 CP 语言官方内置的纯 CP TOML v1.0.0 配置解析与序列化引擎，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，全面覆盖 `Cargo.toml`、`pyproject.toml`、`cap` 配置及 Rust / Python / 云原生配置场景。既可通过 `import { ... } from "cap:toml"` 导入，也支持 `import cap from "cap"; cap.toml.*` 动态懒挂载调用。

```text
┌───────────────────────────────────────────────────────────────────────────────┐
│                     CP 用户层 TOML v1.0.0 引擎 (cap:toml)                       │
├──────────────────────────────────────┬────────────────────────────────────────┤
│ 数据模型与路径访问 (value.cp)          │ 编解码与冲突守卫 (codec.cp)             │
│ TomlDoc / TomlTable / TomlArray      │ TomlParser (4种字符串/数字进制/RFC 3339)│
│ TomlValue / TomlKind / TomlError     │ DuplicateKey / TypeConflict / Depth<=64│
│ get(key) / getPath("a.b.c") / deinit │ TomlSerializer.stringify & toJson      │
└──────────────────────────────────────┴────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/toml/value.cp` | `// Source: cp/lib/toml/value.cp (TOML v1.0.0 Value Model, Error Set & Document Tree)` | `TomlError`、`TomlKind`、`TomlEntry`、`TomlArray`、`TomlTable`、`TomlValue`、`TomlDoc` |
| `lib/toml/codec.cp` | `// Source: cp/lib/toml/codec.cp (TOML v1.0.0 Parser, Conflict Guard & TOML/JSON Serializer)` | `TomlParser`、`TomlSerializer`、`TOML_MAX_DEPTH = 64` |
| `lib/toml/index.cp` | `// Source: cp/lib/toml/index.cp (Unified Exports for cap:toml)` | `cap:toml` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与值模型（`value.cp`）

- **`TomlError`**：`UnexpectedEof`、`UnexpectedChar`、`UnterminatedString`、`InvalidEscape`、`DuplicateKey`、`TypeConflict`、`InvalidNumber`、`InvalidDateTime`、`MaxDepthExceeded`、`OutOfMemory`。
- **`TomlKind`**：`String`、`Integer`、`Float`、`Boolean`、`DateTime`、`Array`、`Table`。
- **`TomlValue`**：
  - 构造：`fromString(alloc, s)`、`fromInt(v)`、`fromFloat(v)`、`fromBool(v)`、`fromDateTime(alloc, s)`、`fromArray(arr)`、`fromTable(tbl)`；
  - 类型探测：`isString()`、`isInt()`、`isFloat()`、`isBool()`、`isDateTime()`、`isArray()`、`isTable()`；
  - 安全取值：`asString()`、`asInt()`、`asFloat()`、`asBool()`、`asDateTime()`、`asArray()`、`asTable()`；
  - 便捷查询：`get(key)`、`getPath("a.b.c")`。
- **`TomlTable` / `TomlArray` / `TomlDoc`**：
  - `TomlTable`：提供 `init(alloc)`、`deinit()`、`len()`、`contains(key)`、`get(key)`、`getPath("a.b.c")`、`put(key, val)`；
  - `TomlArray`：提供 `init(alloc)`、`deinit()`、`len()`、`get(idx)`、`push(val)`；
  - `TomlDoc`：持有根 `TomlTable` 与分配器，`doc.deinit()` 递归释放全部节点与字符串内存。

### 2. TOML v1.0.0 解析器（`TomlParser`）

- **`TomlParser.parse(alloc, src): TomlDoc!`**
  - **注释与换行**：支持 `#` 单行注释、LF / CRLF、UTF-8 BOM 自动剥离；
  - **4 种字符串**：基本字符串 `"..."`（`\b\t\n\f\r\"\\` 与 `\uXXXX`/`\UXXXXXXXX`）、多行基本字符串 `"""..."""`（首行紧邻换行修剪与行尾 `\` 续行修剪）、字面字符串 `'...'`、多行字面字符串 `'''...'''`；
  - **键与表**：裸键 `A-Za-z0-9_-`、引号键、点分键 `a.b.c = 1`、`[table]` 表头、`[[array.of.tables]]` 数组表头、内联表 `{ k = v }`、多行数组 `[ ... ]`（含尾逗号）；
  - **标量**：十进制整数（含 `_` 分隔）、`0x`/`0o`/`0b` 进制整数、浮点（`inf`/`nan`/科学计数法）、布尔、RFC 3339 日期时间；
  - **严格拦截**：重复定义键 `DuplicateKey`、标量上追加子键或表覆盖数组 `TypeConflict`、未闭合字符串 `UnterminatedString`、最大嵌套深度 `TOML_MAX_DEPTH = 64` 守卫 `MaxDepthExceeded`。

### 3. 序列化器（`TomlSerializer`）

- **`TomlSerializer.stringify(alloc, doc): ([]u8)!`**：输出规范 TOML 文本（标量优先、有序子表 `[section]`、数组表 `[[array]]`、自动转义引号键与字符串）；
- **`TomlSerializer.toJson(alloc, doc): ([]u8)!`**：导出严格兼容 RFC 8259 的 JSON 文本（`inf`/`nan` 安全映射为 `null`），可与内置 `JSON` 及 `cap:json` 无缝联动。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { TomlParser, TomlSerializer } from "cap:toml";
import cap from "cap";

pub fn demoTomlConfig(): ! {
    const alloc = std.heap.page_allocator;
    const raw =
        "[package]\n" ++
        "name = \"my-crate\"\n" ++
        "version = \"0.1.0\"\n" ++
        "\n" ++
        "[dependencies]\n" ++
        "serde = { version = \"1.0\", features = [\"derive\"] }\n";

    let doc = TomlParser.parse(alloc, raw)?;
    defer doc.deinit();

    const pkg_name = doc.getPath("package.name")?.asString().?;
    const serde_ver = doc.getPath("dependencies.serde.version")?.asString().?;
    * = pkg_name;
    * = serde_ver;

    const toml_out = TomlSerializer.stringify(alloc, doc)?;
    defer alloc.free(toml_out);

    const json_out = TomlSerializer.toJson(alloc, doc)?;
    defer alloc.free(json_out);

    // 动态挂载调用
    let doc2 = cap.toml.TomlParser.parse(alloc, toml_out)?;
    defer doc2.deinit();
}
```

---

## 零分配 TOML 1.0 转义与词法子模块（收敛自 `toml_escape` / `toml_lexer`）

- **字符串与键名转义 (`escape.cp`)**：`isBareTomlKey`、`formatTomlBasicString`、`formatTomlKey`、`unescapeTomlBasic`。
- **词法扫描器 (`lexer.cp`)**：`TomlTokenKind`、`TomlToken`、`TomlLexer`。
