# CP 标准库：统一数据交换格式引擎 (`cap:interchange`)

## 一、 架构总览与设计准则

`cap:interchange` 是 CP 语言官方内置的纯 CP 统一数据交换格式标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，提供统一的 `InterchangeValue` 模型与 `Interchange` 门面，完整覆盖 **JSON / JSONC / JSON5 / TOML / YAML** 五大格式的解析、序列化与无损互转。既支持 `import { ... } from "cap:interchange"` 静态导入，也支持 `import cap from "cap"; cap.interchange.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                CP 统一数据交换格式引擎 (cap:interchange)                       │
├──────────────────────────────────────┬───────────────────────────────────────┤
│ 统一数据模型 (value.cp)               │ 编解码与桥接 (json5.cp / bridge.cp)    │
│ InterchangeFormat / InterchangeError │ Json5Codec (JSON / JSONC / JSON5)     │
│ InterchangeValue / InterchangeEntry  │ TomlYamlBridge (TOML / YAML 桥接互转) │
│ getPath("a.b.0") / clone / deinit    │ 深度上限守卫 INTERCHANGE_MAX_DEPTH=64  │
└──────────────────────────────────────┴───────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/interchange/value.cp` | `// Source: cp/lib/interchange/value.cp (...)` | `InterchangeFormat`、`InterchangeError`、`InterchangeKind`、`InterchangeValue`、`INTERCHANGE_MAX_DEPTH = 64` |
| `lib/interchange/json5.cp` | `// Source: bun/src/interchange/json5.cp -> cp/lib/interchange/json5.cp (...)` | 纯 CP JSON / JSONC / JSON5 解析器与序列化器 |
| `lib/interchange/bridge.cp` | `// Source: cp/lib/interchange/bridge.cp (...)` | 桥接 `cap:toml` / `cap:yaml` 解析与纯 CP TOML / YAML 序列化器 |
| `lib/interchange/interchange.cp` | `// Source: bun/src/interchange/interchange.cp -> cp/lib/interchange/interchange.cp (...)` | `Interchange` 统一门面（`detectFormat`、`parse`、`stringify`、`convert`） |
| `lib/interchange/index.cp` | `// Source: cp/lib/interchange/index.cp (...)` | `cap:interchange` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 格式枚举与错误集合（`value.cp`）

- **`InterchangeFormat`**：`Json`、`Jsonc`、`Json5`、`Toml`、`Yaml`。
- **`InterchangeError`**：`UnsupportedFormat`、`UnexpectedEof`、`UnexpectedChar`、`UnexpectedToken`、`UnterminatedString`、`UnterminatedComment`、`InvalidEscape`、`InvalidNumber`、`DuplicateKey`、`TypeConflict`、`InvalidIndentation`、`TabIndentationForbidden`、`MaxDepthExceeded`、`TrailingData`、`OutOfMemory`。
- **`InterchangeValue`**：
  - 类型：`Null`、`Bool`、`Int`、`Float`、`String`、`Array`、`Object`；
  - 构造：`initNull()`、`fromBool(v)`、`fromInt(v)`、`fromFloat(v)`、`fromString(alloc, s)`、`fromArray(items)`、`fromObject(entries)`；
  - 类型探测与安全取值：`isNull()`、`isBool()`、`isInt()`、`isFloat()`、`isNumber()`、`isString()`、`isArray()`、`isObject()`；`asBool()`、`asInt()`、`asFloat()`、`asString()`、`asArray()`、`asObject()`；
  - 路径与比较：`get(key)`、`getIndex(idx)`、`getPath("a.b.0")`、`equals(other)`、`clone(alloc)`、`deinit(alloc)`。

### 2. 统一门面（`interchange.cp`）

- **`detectFormat(path_or_ext): InterchangeFormat?`**：根据文件路径或扩展名自动探测格式（支持 `.json`、`.jsonc`、`.json5`、`.toml`、`.yaml`、`.yml`，大小写不敏感）。
- **`parse(alloc, input, format): InterchangeValue!`**：按指定格式解析为统一 `InterchangeValue`（`deinit(alloc)` 递归释放全量内存，支持 `std.testing.allocator` 零泄漏）。
- **`parseAuto(alloc, input, path_or_ext): InterchangeValue!`**：自动探测格式并解析。
- **`stringify(alloc, val, format): ([]u8)!`** / **`stringifyPretty(alloc, val, format, pretty): ([]u8)!`**：序列化为目标格式文本。
- **`convert(alloc, input, from_fmt, to_fmt): ([]u8)!`**：一键完成跨格式无损互转（如 TOML -> JSON、YAML -> TOML、JSON5 -> JSON）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    Interchange,
    InterchangeFormat,
    detectFormat,
    parse,
    stringify,
} from "cap:interchange";
import cap from "cap";

pub fn demoInterchange(): ! {
    const alloc = std.heap.page_allocator;
    const json5_src = "{ // config\n  name: 'cp-app', port: 0x1F90, ratio: .5, }";

    const fmt = detectFormat("config.json5").?;
    let val = parse(alloc, json5_src, fmt)?;
    defer val.deinit(alloc);

    const json_out = stringify(alloc, val, .Json)?;
    defer alloc.free(json_out);

    const toml_out = cap.interchange.Interchange.convert(alloc, json5_src, .Json5, .Toml)?;
    defer alloc.free(toml_out);
}
```
