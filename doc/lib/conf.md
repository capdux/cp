# `cap:conf` — 统一配置交换引擎（YAML 1.2 / TOML 1.0 / INI / JSON5 & `ConfGetter`）

> 参考来源：`/data/dev/bun/docs/runtime/{toml,yaml,json5,environment-variables}.mdx`, `/data/dev/bun/src/interchange/`, `/data/dev/bun/src/ini/ini.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/conf/`  
> **导入协议**: `import { ConfTag, ConfDateTime, ConfDateTimeKind, ConfValue, YamlParser, YamlStream, YamlDoc, YAML, TomlParser, TomlDoc, TOML, Ini, JSON5, DotEnv, ConfGetter, ConfValueParser } from "cap:conf";`

---

## 一、 架构概述与设计亮点

`cap:conf` 是 CP 标准库的统一配置与数据交换子系统，将 Bun 内部的四大核心配置解析器（**TOML 1.0**、**YAML 1.2**、**INI**、**JSON5**）与统一 AST (`ConfValue`)、强类型配置提取器（**`ConfGetter`**）整合为一套零冗余、类型安全的原生模块：

1. **统一配置 AST 与 RFC 3339 时间模型 (`ConfValue` / `ConfDateTime`)**：提供 `Null`/`Bool`/`Int`/`Float`/`String`/`DateTime`/`Array`/`Object` 八态递归值树、`getPath("a.b.0.c")` 点分/索引路径查询、`clone` 深拷贝与 `toJsonValue` 跨模块桥接；`ConfDateTime` 完整校验 RFC 3339 四种格式（`OffsetDateTime`、`LocalDateTime`、`LocalDate`、`LocalTime`）及闰年日历合法性。
2. **工业级 TOML 1.0 解析器与序列化器 (`TomlParser` / `TomlDoc` / `TOML`，对标 `toml-rs/toml` MIT/Apache-2.0 加固)**：严格执行 TOML v1.0.0 语义——裸键/引号键/空引号键/深层点分键、重复键与显式表重定义严格拒绝、Inline Table 单行冻结不可追加（禁止换行与尾随逗号）、`[[array_of_tables]]` 与静态数组隔离、64 层最大嵌套深度防护、多行字符串 `"""..."""` / `'''...'''`、`\u`/`\U` 代理对（`0xD800..0xDFFF`）与控制字符拒绝、禁止数值前导零与非法下划线、有限浮点溢出拒绝、RFC 3339 闰年日历校验、**`cap:simd` 热路径加速**（注释跳行、空白跳过、单行/多行字符串块级扫描），并支持 `TomlParser.stringify`（全控制字符转义）双向序列化与 100% 向后兼容 `TomlDoc`（`TOML` / `toml`）。
3. **工业级 YAML 1.2 解析器与序列化器 (`YamlParser` / `YamlStream` / `YamlDoc` / `YAML`)**：支持多文档流（`---` / `...`、`%YAML` / `%TAG`）、锚点与别名（`&anchor` / `*alias`）、合并键（`<<: *base` 与 `<<: [*a, *b]`）、块标量（`|`/`|-`/`|+`/`>`/`>-`/`>+` 及显式缩进指示）、流式集合（`[...]` / `{...}`）、显式标签（`!!str`/`!!int`/`!!float`/`!!bool`/`!!null`/`!!seq`/`!!map`）、单双引号转义（`\x` / `\u` / `\U`）以及 `YamlParser.stringify` / `stringifyStream` 双向序列化，同时保留向后兼容的 `YamlDoc`（`YAML` / `yaml`）。
4. **INI 配置与环境变量展开引擎 (`Ini` / `ini`)**：支持全局属性、`[section]` 与 `[section.nested]` 节管理、`;` 与 `#` 注释、双引号转义与 `${ENV_VAR}` 系统环境变量自动展开，以及动态增删改与 `toString()` 序列化。
5. **JSON5 全功能 AST 解析与注释剥离器 (`JSON5`)**：支持 `//` 单行注释、`/* */` 多行块注释、对象与数组尾随逗号（Trailing Commas）、无引号标识符键名、单引号字符串、十六进制数值 `0x1F90`、前导正号 `+100`、`Infinity` / `NaN`，以及将 JSON5 降级净化为标准 JSON 的零分配预处理器 `stripCommentsAndTrailingCommas`。
6. **统一类型安全配置提取器 (`ConfGetter` / `ConfValueParser`)**：跨越不同配置格式，将原始字符串或可选切片 `([]const u8)?` 安全解析为布尔值（`true`/`yes`/`on`/`1`）、端口号（`1..65535`）、时间跨度（`150ms`, `30s`, `5m`, `2h` -> 毫秒）与字节容量（`512b`, `64kb`, `16mb`, `2gb` -> 字节数）。

---

## 二、 五大配置格式与 `ConfGetter` 核心 API 矩阵

### 1. 配置解析器 API 对照表

| 配置格式 | 核心结构体 | 解析入口 | 读取与路径查询 | 类型化读取 & 序列化 |
| :--- | :--- | :--- | :--- | :--- |
| **YAML 1.2 (AST)** | `YamlParser` / `YamlStream` | `YamlParser.parse(alloc, src)` / `parseStream(alloc, src)` | `doc.getPath("service.tls")` / `stream.getDoc(i)` | `asInt`, `asBool`, `asString`, `stringify`, `stringifyStream` |
| **YAML (兼容)** | `YamlDoc` (`YAML`) | `YamlDoc.parse(src)` / `parseTree(alloc, src)` | `doc.get(sec, key)` / `getRoot(key)` | `getInt`, `getBool`, `getOr`, `hasSection`, `hasKey` |
| **TOML 1.0 (AST)** | `TomlParser` | `TomlParser.parse(alloc, src)` | `root.getPath("products.1.name")` / `root.get(k)` | `asInt`, `asFloat`, `asDateTime`, `asArray`, `TomlParser.stringify` |
| **TOML (兼容)** | `TomlDoc` (`TOML`) | `TomlDoc.parse(src)` / `parseTree(alloc, src)` | `doc.get(table, key)` / `getRoot(key)` | `getInt`, `getFloat`, `getBool`, `getDateTime`, `getArray`, `toString` |
| **INI** | `Ini` (`ini`) | `Ini.parse(src)` | `conf.get(sec, key)` / `getGlobal(key)` | `getInt`, `getFloat`, `getBool`, `set`, `remove`, `toString` |
| **JSON5** | `JSON5` | `JSON5.parse(alloc, src)` | `val.get(key)` -> `asString()` | `asNumber()`, `asBool()`, `getIndex(i)`, `stripCommentsAndTrailingCommas` |
| **DotEnv** | `DotEnv` (`dotenv`) | `DotEnv.parse(src)` | `env.get(key)` / `getOr(key, def)` | `env.has(key)` |

### 2. `ConfGetter` (`ConfValueParser`) 强类型提取接口表

| 方法签名 | 支持的输入格式示例 | 返回值示例 | 说明 |
| :--- | :--- | :--- | :--- |
| `ConfGetter.parseBool(raw)` / `getBoolOr(opt, def)` | `"true"`, `"yes"`, `"on"`, `"1"`, `"false"`, `"no"`, `"off"`, `"0"` | `true` / `false` | 大小写不敏感布尔语义识别 |
| `ConfGetter.parsePort(raw)` / `getPortOr(opt, def)` | `"8080"`, `"443"` | `8080` (`u16`) | 严格校验 `1 <= port <= 65535`，非法端口返回 `null` 或 `def` |
| `ConfGetter.parseDurationMs(raw)` / `getDurationMsOr(opt, def)` | `"500ms"`, `"15s"`, `"5m"`, `"2h"` | `15000` (`u64`) | 自动换算为毫秒（`ms`） |
| `ConfGetter.parseByteSize(raw)` / `getByteSizeOr(opt, def)` | `"1024b"`, `"64kb"`, `"16mb"`, `"2gb"` | `16777216` (`u64`) | 按 1024 进制（KiB/MiB/GiB）自动换算为字节数 |
| `ConfGetter.parseU64(raw)` / `getU64Or(opt, def)` | `"10000"` | `10000` (`u64`) | 无符号 64 位整数解析 |
| `ConfGetter.parseI64(raw)` / `getI64Or(opt, def)` | `"-42"` | `-42` (`i64`) | 有符号 64 位整数解析 |
| `ConfGetter.parseF64(raw)` / `getF64Or(opt, def)` | `"3.14159"` | `3.14159` (`f64`) | 64 位双精度浮点数解析 |

---

## 三、 代码示例

### 1. 完整 TOML 1.0 AST 解析、RFC 3339 时间与双向序列化 (`TomlParser` & `TomlDoc`)

```typescript
const std = @import("std");
import { TomlParser, TomlDoc, ConfDateTime, ConfDateTimeKind } from "cap:conf";

fn demoToml(): !void {
    const alloc = std.heap.page_allocator;
    const toml_src =
        "point = { x = 10, y = 20 }\n" +
        "nested.dotted.val = \"deep\"\n" +
        "\"127.0.0.1\" = \"loopback\"\n" +
        "created_utc = 1979-05-27T07:32:00Z\n" +
        "local_d = 2024-02-29\n" +
        "\n" +
        "[[products]]\n" +
        "name = \"Hammer\"\n" +
        "sku = 738594937\n" +
        "\n" +
        "[[products]]\n" +
        "name = \"Nail\"\n" +
        "meta = { color = \"gray\", weight = 0.05 }\n";

    // 1. 完整 AST 解析与点分/索引路径查询
    const root = try TomlParser.parse(alloc, toml_src);
    const px = root.getPath("point.x").?.asInt().?;                      // 10
    const p1_color = root.getPath("products.1.meta.color").?.asString().?; // "gray"
    const dt = root.get("created_utc").?.asDateTime().?;                 // OffsetDateTime, is_utc=true

    // 日历合法性校验（2025 非闰年返回 null）
    const invalid = ConfDateTime.parse("2025-02-29");                    // null

    // 双向序列化回标准 TOML 文本
    const encoded = try TomlParser.stringify(alloc, root);

    // 2. 向后兼容零分配扁平查询 API (TomlDoc)
    let doc = TomlDoc.parse(toml_src);
    let x_val = doc.getInt("point", "x", 0);                             // 10
    let utc = doc.getDateTime("", "created_utc").?;
}
```

### 2. 完整 YAML 1.2 多文档流、锚点/别名、合并键与双向序列化 (`YamlParser` & `YamlDoc`)

```typescript
const std = @import("std");
import { YamlParser, YamlDoc } from "cap:conf";

fn demoYaml12(): !void {
    const alloc = std.heap.page_allocator;
    const yaml_src =
        "%YAML 1.2\n" +
        "---\n" +
        "defaults: &base\n" +
        "  timeout: 30\n" +
        "  retries: 3\n" +
        "service:\n" +
        "  <<: *base\n" +
        "  timeout: 60\n" +
        "  notes: |-\n" +
        "    line1\n" +
        "    line2\n" +
        "  tags: [\"api\", !!str 42]\n" +
        "...\n" +
        "---\n" +
        "- item: second\n";

    // 多文档流解析
    const stream = try YamlParser.parseStream(alloc, yaml_src);
    const doc0 = stream.getDoc(0).?;
    const timeout = doc0.getPath("service.timeout").?.asInt().?; // 60（显式键覆盖合并键）
    const retries = doc0.getPath("service.retries").?.asInt().?; // 3（继承自 *base）
    const tag1 = doc0.getPath("service.tags.1").?.asString().?;  // "42"

    // 双向序列化与重解析
    const dumped = try YamlParser.stringify(alloc, doc0);
    const reparsed = try YamlParser.parse(alloc, dumped);

    // 向后兼容零分配 API
    const legacy = YamlDoc.parse("server:\n  host: 127.0.0.1\n  port: 8080\n");
    const port = legacy.getInt("server", "port", 0); // 8080
}
```

### 3. INI 解析结合 `ConfGetter` 统一类型安全提取

```typescript
import { Ini, YamlDoc, ConfGetter } from "cap:conf";

fn demoIniYamlAndGetter(): void {
    const ini_src =
        "[gateway]\n" +
        "listen_port = 8443\n" +
        "keepalive = 30s\n" +
        "max_payload = 32mb\n" +
        "tls_enabled = on\n";

    let ini_doc = Ini.parse(ini_src);

    // 使用 ConfGetter 直接从可选切片安全提取强类型配置值
    let port = ConfGetter.getPortOr(ini_doc.get("gateway", "listen_port"), 8080);         // 8443 (u16)
    let timeout_ms = ConfGetter.getDurationMsOr(ini_doc.get("gateway", "keepalive"), 0);  // 30000 (u64)
    let max_bytes = ConfGetter.getByteSizeOr(ini_doc.get("gateway", "max_payload"), 0);   // 33554432 (u64)
    let tls = ConfGetter.getBoolOr(ini_doc.get("gateway", "tls_enabled"), false);         // true (bool)

    // YAML 块级配置解析
    const yaml_src =
        "---\n" +
        "environment: production\n" +
        "redis:\n" +
        "  host: 127.0.0.1\n" +
        "  port: 6379\n";

    let yaml_doc = YamlDoc.parse(yaml_src);
    let r_host = yaml_doc.getOr("redis", "host", "localhost"); // "127.0.0.1"
    let r_port = yaml_doc.getInt("redis", "port", 6379);       // 6379
}
```

### 4. JSON5 解析与注释/尾随逗号净化

```typescript
const std = @import("std");
import { JSON5 } from "cap:conf";

fn demoJson5(): !void {
    const json5_src =
        "{\n" +
        "  // 支持无引号键名、单引号字符串与十六进制数字\n" +
        "  appName: 'cp-server',\n" +
        "  listenPort: 0x1F90,\n" +
        "  tags: [ 'edge', 'native', ],\n" +
        "}";

    let ast = try JSON5.parse(std.heap.page_allocator, json5_src);
    if (p := ast.get("listenPort")) {
        let port_num = p.asNumber().?; // 8080.0
    }

    // 预处理器：零分配剥离注释与尾随逗号
    let out_buf: [512]u8 = undefined;
    let clean_json = JSON5.stripCommentsAndTrailingCommas(json5_src, &out_buf);
}
```

---

## 五、 上游 Bun 行为契约与边界注意事项

1. **`.env` 分层加载优先级约定（参考 `/data/dev/bun/docs/runtime/environment-variables.mdx`）**：
   - 上游 Bun 按递增优先级依次读取 `.env` < `.env.<NODE_ENV>`（`production`/`development`/`test`）< `.env.local` < 进程显式环境变量；在 CP 应用中通过 `DotEnv` 分层加载多份配置文件时推荐沿用该优先级，避免本地覆盖泄露至生产配置。
2. **YAML 1.2 安全与多文档边界（对标 `Ethiraric/yaml-rust2` 加固）**：
   - `YamlParser.parseStream` 严格以 `---` / `...` 切分多文档流（支持 `%YAML` / `%TAG` 指令与跨文档锚点隔离），支持块标量全量 chomping（`|`、`|-`、`|+`、`>`、`>-`、`>+` 与显式缩进 `1..9`）、Flow 集合（`[...]` / `{...}`）、显式标签（`!!str`/`!!int`/`!!float`/`!!bool`/`!!null`）、YAML 1.2 数值（`0o`/`0x`/`.inf`/`-.inf`/`.nan`，且未带点号的 `nan`/`inf`/`Infinity` 严格保留为字符串）以及合并键（`<<: *a` 与 `<<: [*a, *b]`）。
   - 安全防护：`YAML_MAX_DEPTH = 64` 递归深度上限（`DepthExceeded`）、循环引用实时拦截（`CircularAlias`）、指数级别名展开预算上限（`YAML_MAX_ALIAS_EXPANSIONS = 256`、`YAML_MAX_EXPANDED_NODES = 16384` -> `AliasExpansionLimitExceeded`）及块级 Tab 缩进禁止（`TabIndentationForbidden`）。
   - 热路径 `cap:simd` 加速：行尾/注释扫描（`scanLineEndSimd`）、缩进空格计数（`countIndentSpacesSimd`）与单/双引号字符串快扫（`scanSingleQuotedStopSimd` / `scanDoubleQuotedStopSimd`）全面接入 `cap:simd`，并与 `*Scalar` 参考实现 100% 等价。
3. **TOML 1.0 与 RFC 3339 严格校验（对标 `toml-rs/toml` 加固）**：
   - `TomlParser.parse` 内置 Arena 节点池构建器与 64 层深度上限（`DepthExceeded`），严格拒绝重复键（`DuplicateKey`）、显式/点分表重定义（`TableRedefined`）、Inline Table 冻结后追加（`FrozenTable`）、多行字符串作为键、UTF-16 代理对与未转义控制字符（`InvalidControlChar`）、数值前导零/非法下划线/有限浮点溢出（`InvalidNumber`）及非闰年/越界 RFC 3339 时间（`InvalidDateTime`）；
   - 注释跳行、空白跳过与单行/多行基础/字面量字符串扫描热路径全面接入 `cap:simd`（`Simd.indexOfByte` / `Simd.indexOfAny2` / `Simd.indexOfAny4` / `Simd.skipWhitespace`），并暴露 `TomlParser.scanBasicStringStopSimd` / `scanBasicStringStopScalar` 便于向量化等价校验。
4. **JSON5 净化快路径**：
   - 对仅需兼容注释与尾随逗号的配置文件（如 `tsconfig.json`），优先调用零分配 `JSON5.stripCommentsAndTrailingCommas(src, out_buf)` 将其净化为标准 JSON 后交由 `cap:json` 极速解析。

---

## 六、 扩展配置格式与 JSON 生态家族（零冲突归并）

以下 17 个配置/结构化文本与 JSON 扩展模块已统一收敛至 `lib/conf/` 并零冲突导出至 `cap:conf`：

### 1. Bun 吸收扩展配置模块（5 个）

| 子模块文件 | 职责说明 | 核心导出符号 |
| :--- | :--- | :--- |
| `jsonc.cp` | JSONC 注释与尾随逗号净化 | `stripJsonc`, `stripJsoncPreserveLines`, `hasJsoncComments`, `countJsoncTrailingCommas` |
| `editorconfig.cp` | `.editorconfig` 解析与属性计算 | `resolveEditorConfig`, `matchSectionPattern`, `EditorConfigProps`, `IndentStyle`, `EndOfLine`, `Charset` |
| `frontmatter.cp` | Markdown YAML/TOML Frontmatter 提取 | `splitFrontmatter`, `getFrontmatterField`, `FrontmatterKind`, `FrontmatterSplit` |
| `makefileParse.cp` | Makefile 目标与变量解析 | `MakeRuleIter`, `MakeVarIter`, `isPhonyTarget`, `MakeRule`, `MakeVar`, `MakeAssignOp` |
| `tsconfigPaths.cp` | `tsconfig.json` paths 映射解析 | `TsConfigPaths`, `PathMapping`, `ResolvedCandidate`, `expandConfigDir` |

### 2. Pure-CP 现代配置与 JSON 扩展模块（12 个）

| 子模块文件 | 职责说明 | 核心导出符号 |
| :--- | :--- | :--- |
| `hjson.cp` | Hjson 词法分析与 JSON 转换 | `hjsonToJson`, `isValidHjson`, `stripHjsonComments`, `HjsonToken`, `HjsonTokenKind` |
| `jsonl.cp` | 零分配 JSONL 流式行分帧器 | `JsonlIter`, `scanCompleteJsonlChunk`, `isBalancedJsonRecord`, `appendJsonlRecord`, `JsonlChunkResult` |
| `ndjson.cp` | NDJSON 流式解析校验与写入 | `NdjsonStreamParser`, `NdjsonWriter`, `parseAll`, `stringifyAll`, `NdjsonRecord`, `NdjsonError` |
| `jsonpatch.cp` | RFC 6901 Pointer & RFC 6902 Patch | `applyPatch`, `createDiffPatch`, `resolvePointer`, `jsonValuesEqual`, `escapeSegment`, `unescapeSegment`, `PatchOpKind` |
| `jsonschema.cp` | JSON Schema Draft-07/2020-12 校验器 | `SchemaValidator`, `ValidationResult`, `ValidationIssue`, `JsonSchemaError`, `JSONSCHEMA_MAX_DEPTH` |
| `kdl.cp` | KDL 文档词法与节点扫描器 | `KdlIter`, `KdlNodeIter`, `validateKdl`, `KdlToken`, `KdlNodeSummary` |
| `hcl.cp` | HCL / Terraform 块扫描与校验 | `HclIter`, `HclBlockIter`, `validateHcl`, `HclToken`, `HclBlockHeader` |
| `cue.cp` | CUE 配置语言词法与校验 | `CueIter`, `validateCue`, `extractCuePackage`, `CueToken`, `CueTokenKind` |
| `ron.cp` | Rust Object Notation (RON) 解析 | `RonIter`, `validateRon`, `extractRonField`, `RonToken`, `RonTokenKind` |
| `nix.cp` | Nix 表达式与 Flake 扫描器 | `NixIter`, `NixAttrIter`, `validateNixDelimiters`, `NixToken`, `NixAttrBinding` |
| `plist.cp` | Apple XML Plist 解析与格式化 | `isXmlPlist`, `PlistDictIter`, `findPlistString`, `findPlistInt`, `findPlistBool`, `formatPlistStringEntry` |
| `crontab.cp` | 5-Field & Macro Cron 解析与下次触发 | `CronExpr`, `DateTime`, `CronError` |

> **重叠对照参考归档说明**：`json5_.cp`、`json5_lexer_.cp`、`dotenv_parse_.cp`、`json_patch_.cp` 为与权威实现功能重叠的对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/conf/index.cp` 冲突导出。

---

## 收敛子模块深度文档

- [crontab.md](conf/crontab.md)
- [cue.md](conf/cue.md)
- [hcl.md](conf/hcl.md)
- [hjson.md](conf/hjson.md)
- [jsonl.md](conf/jsonl.md)
- [jsonpatch.md](conf/jsonpatch.md)
- [jsonschema.md](conf/jsonschema.md)
- [kdl.md](conf/kdl.md)
- [ndjson.md](conf/ndjson.md)
- [nix.md](conf/nix.md)
- [plist.md](conf/plist.md)
- [ron.md](conf/ron.md)
