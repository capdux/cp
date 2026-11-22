# CP 标准库：YAML 1.2 解析、类型安全访问、序列化与 JSON 互转 (`cap:yaml`)

> 参考来源：`/data/dev/bun/docs/runtime/yaml.mdx`, `/data/dev/bun/src/interchange/yaml.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:yaml` 是 CP 语言官方内置的工业生产级纯 CP YAML 1.2 核心子集解析与序列化标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，既可通过 `import { ... } from "cap:yaml"` 导入，也支持 `import cap from "cap"; cap.yaml.*` 动态挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                     CP 配置解析与数据互转引擎 (cap:yaml)                  │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ DOM 与路径查询     │ YAML 1.2 核心解析    │ 规范序列化与 JSON 导出         │
│ YamlValue / Kind  │ YamlParser          │ YamlSerializer.stringify      │
│ YamlDoc / getPath │ 块/流集合 & 块标量   │ YamlSerializer.toJson         │
│ 锚点/别名 & RAII   │ Tab/重复键/深度防护  │ RFC 8259 合规转义              │
└───────────────────┴─────────────────────┴───────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/yaml/value.cp` | `// Source: cp/lib/yaml/value.cp (...)` | `YamlError`、`YamlKind`、`YamlEntry`、`YamlValue`、`YamlDoc`（递归 `deinit`、`get`、`getPath`、安全类型转换） |
| `lib/yaml/codec.cp` | `// Source: cp/lib/yaml/codec.cp (...)` | `YamlParser`（块/流集合、引号/块标量、锚点/别名、安全防护）与 `YamlSerializer`（`stringify` / `toJson`） |
| `lib/yaml/index.cp` | `// Source: cp/lib/yaml/index.cp (...)` | `cap:yaml` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与值类型（`value.cp`）

- **`YamlError`**：`UnexpectedEof`、`InvalidIndentation`、`TabIndentationForbidden`、`DuplicateKey`、`UnterminatedString`、`UnterminatedFlow`、`InvalidAlias`、`MaxDepthExceeded`、`OutOfMemory`。
- **`YamlKind`**：`Null`、`Bool`、`Int`、`Float`、`String`、`Sequence`、`Mapping`。
- **`YamlValue`**：
  - 类型判断：`isNull()`、`isBool()`、`isInt()`、`isFloat()`、`isString()`、`isSeq()`、`isMap()`；
  - 安全转换：`asBool(): bool?`、`asInt(): i64?`、`asFloat(): f64?`、`asString(): ([]const u8)?`、`asSeq(): ([]YamlValue)?`、`asMap(): ([]YamlEntry)?`；
  - 索引与路径查询：`get(key)`、`getIndex(idx)`、`getPath("jobs.build.runs-on")`（支持映射键与序列数字索引混合路径，如 `"jobs.build.steps.0.name"`）；
  - 生命周期：`clone(alloc)`、`deinit(alloc)`。
- **`YamlDoc`**：`init(alloc)`、`deinit()`、`get(key)`、`getIndex(idx)`、`getPath(path)`，支持单文档 `root` 与多文档 `documents`。

### 2. 解析器与序列化器（`codec.cp`）

- **`YamlParser`**：
  - `YamlParser.parse(alloc, input): YamlDoc!`：解析单个 YAML 文档；
  - `YamlParser.parseAll(alloc, input): YamlDoc!`：解析多文档流（`---` / `...` 分隔）；
  - 支持行内与整行 `#` 注释、严格空格缩进校验（禁止行首 `\t` 触发 `TabIndentationForbidden`）、64 层最大嵌套深度守卫（`MaxDepthExceeded`）、同级映射重复键拦截（`DuplicateKey`）；
  - 支持块映射 `key: val`、嵌套块序列 `- item` 与序列内嵌映射 `- key: val`、流式集合 `[a, b]` / `{ k: v }`（可嵌套、可跨行）；
  - 支持双引号 `"..."`（含转义）、单引号 `'...'`（含 `''`）、块标量 `|` / `|-` / `|+` / `>` / `>-` / `>+`、裸标量类型推断（`null`/`~`/`true`/`false`/十进制与 `0x` 十六进制整数/浮点）；
  - 支持锚点 `&anchor` 与别名 `*alias` 安全深拷贝展开。
- **`YamlSerializer`**：
  - `YamlSerializer.stringify(alloc, doc): ([]const u8)!`：输出规范 2 空格缩进 YAML 文本，自动对特殊字符串加引号以保证无损往返；
  - `YamlSerializer.toJson(alloc, doc): ([]const u8)!`：导出合规 RFC 8259 JSON 字符串。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { YamlParser, YamlSerializer } from "cap:yaml";
import cap from "cap";

pub fn demoYaml(): ! {
    const alloc = std.heap.page_allocator;
    const raw =
        "name: cp-ci\n" ++
        "on: [push, pull_request]\n" ++
        "jobs:\n" ++
        "  build:\n" ++
        "    runs-on: ubuntu-latest\n";

    let doc = YamlParser.parse(alloc, raw)?;
    defer doc.deinit();

    const runner = doc.getPath("jobs.build.runs-on").?.asString().?;
    const json_out = YamlSerializer.toJson(alloc, doc)?;
    defer alloc.free(json_out);

    let dyn_doc = cap.yaml.YamlParser.parse(alloc, "port: 8080\n")?;
    defer dyn_doc.deinit();
    * = runner;
}
```

---

## 零分配 YAML 1.2 标量解析与转义子模块（收敛自 `yaml_escape` / `yaml_scalar`）

- **标量引号与转义 (`escape.cp`)**：`needsYamlQuotes`、`quoteYamlSingle`、`quoteYamlDouble`、`formatYamlScalar`、`unescapeYamlDouble`。
- **Core Schema 标量解析 (`scalar.cp`)**：`YamlScalarKind`、`YamlScalar`、`parseCoreScalar`、`unescapeDoubleQuoted`、`unescapeSingleQuoted`、`needsQuoting`、`formatSafeScalar`。

---

## 上游 Bun YAML 1.2 行为契约与安全边界注意事项

1. **YAML 1.2 Core Schema 与 1.1 挪威问题免疫（`parseCoreScalar`）**：
   - 严格遵循 YAML 1.2 Core Schema：仅 `true`/`True`/`TRUE`、`false`/`False`/`FALSE` 解析为布尔值，**不再**将 `yes`/`no`/`on`/`off`/`y`/`n` 误判为布尔（彻底消除 YAML 1.1 经典“挪威国码 `NO` 变 `false`”陷阱）。
2. **禁止 Tab 缩进与重复键防护**：
   - 块级缩进中出现制表符 `\t` 直接报错拒绝；同层映射中重复键与过深嵌套均被拦截，避免配置静默覆盖或资源耗尽。
3. **安全标量序列化（`needsYamlQuotes` / `formatSafeScalar`）**：
   - 写回 YAML 时对以特殊字符（`:`、`#`、`@`、`&`、`*`、`!`、`-`）开头或与 `null`/`true`/`false`/数字字面量冲突的字符串自动加引号，确保往返解析类型不漂移。
