# `cap:conf` — 统一配置交换引擎（TOML / INI / YAML / JSON5 & `ConfGetter`）

> **上游溯源**: `/data/dev/bun/src/ini/ini.cp` & `/data/dev/bun/src/interchange/` (`toml.cp`, `yaml.cp`, `json5.cp`)  
> **模块路径**: `lib/conf/`  
> **导入协议**: `import { Ini, TomlDoc, TOML, YamlDoc, YAML, JSON5, DotEnv, ConfGetter, ConfValueParser } from "cap:conf";`

---

## 一、 架构概述与设计亮点

`cap:conf` 是 CP 标准库的统一配置与数据交换子系统，将 Bun 内部的四大核心配置解析器（**TOML**、**INI**、**YAML**、**JSON5**）与强类型配置提取器（**`ConfGetter`**）整合为一套零冗余、类型安全的原生模块：

1. **TOML 工业级文档模型 (`TomlDoc` / `TOML`)**：支持 `[table]` 标准表、`[[array_of_tables]]` 表数组、`a.b.c = "val"` 点分嵌套键、十进制下划线分隔符 `1_000`、十六进制 `0xDEAD`、八进制 `0o755`、二进制 `0b1010`、特殊浮点数（`inf`, `nan`）、多行字符串 `"""..."""`、内联数组 `[ "a", "b" ]` 以及双向序列化 `toString()`。
2. **INI 配置与环境变量展开引擎 (`Ini` / `ini`)**：支持全局属性、`[section]` 与 `[section.nested]` 节管理、`;` 与 `#` 注释、双引号转义与 `${ENV_VAR}` 系统环境变量自动展开，以及动态增删改与 `toString()` 序列化。
3. **轻量级零分配 YAML 解析器 (`YamlDoc` / `YAML`)**：支持 `---` / `...` 文档边界、缩进块级 Section 自动识别、键值提取与类型转换（`getInt`, `getBool`, `getOr`）。
4. **JSON5 全功能 AST 解析与注释剥离器 (`JSON5`)**：支持 `//` 单行注释、`/* */` 多行块注释、对象与数组尾随逗号（Trailing Commas）、无引号标识符键名、单引号字符串、十六进制数值 `0x1F90`、前导正号 `+100`、`Infinity` / `NaN`，以及将 JSON5 降级净化为标准 JSON 的零分配预处理器 `stripCommentsAndTrailingCommas`。
5. **统一类型安全配置提取器 (`ConfGetter` / `ConfValueParser`)**：跨越不同配置格式，将原始字符串或可选切片 `([]const u8)?` 安全解析为布尔值（`true`/`yes`/`on`/`1`）、端口号（`1..65535`）、时间跨度（`150ms`, `30s`, `5m`, `2h` -> 毫秒）与字节容量（`512b`, `64kb`, `16mb`, `2gb` -> 字节数）。

---

## 二、 四大配置格式与 `ConfGetter` 核心 API 矩阵

### 1. 配置解析器 API 对照表

| 配置格式 | 核心结构体 | 解析入口 | 读取字符串 | 类型化读取 & 序列化 |
| :--- | :--- | :--- | :--- | :--- |
| **TOML** | `TomlDoc` (`TOML`) | `TomlDoc.parse(src)` | `doc.get(table, key)` / `getRoot(key)` | `getInt`, `getFloat`, `getBool`, `getArray`, `toString` |
| **INI** | `Ini` (`ini`) | `Ini.parse(src)` | `conf.get(sec, key)` / `getGlobal(key)` | `getInt`, `getFloat`, `getBool`, `set`, `remove`, `toString` |
| **YAML** | `YamlDoc` (`YAML`) | `YamlDoc.parse(src)` | `doc.get(sec, key)` / `getRoot(key)` | `getInt`, `getBool`, `getOr`, `hasSection`, `hasKey` |
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

### 1. TOML 解析、数组提取与双向序列化

```typescript
const std = @import("std");
import { TomlDoc } from "cap:conf";

fn demoToml(): void {
    const toml_src =
        "title = \"CP Engine\"\n" +
        "\n" +
        "[network]\n" +
        "port = 9000\n" +
        "max_clients = 10_000\n" +
        "flags = 0b1010\n" +
        "hosts = [ \"10.0.0.1\", \"10.0.0.2\" ]\n" +
        "\n" +
        "[network.tls]\n" +
        "enabled = true\n";

    let doc = TomlDoc.parse(toml_src);

    let title = doc.getRoot("title").?;                    // "CP Engine"
    let max_c = doc.getInt("network", "max_clients", 0);   // 10000
    let flags = doc.getInt("network", "flags", 0);         // 10
    let tls_on = doc.getBool("network.tls", "enabled", false); // true

    let hosts = doc.getArray("network", "hosts");
    // hosts.len => 2, hosts[0] => "10.0.0.1"

    // 动态修改并序列化回 TOML 文本
    doc.set("network", "port", "9001");
    let encoded = doc.toString();
}
```

### 2. INI 与 YAML 解析结合 `ConfGetter` 统一类型安全提取

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

### 3. JSON5 解析与注释/尾随逗号净化

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
