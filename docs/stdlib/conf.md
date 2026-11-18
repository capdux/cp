# `cap:conf` 统一配置与数据交换子系统

`cap:conf` 为 CP 语言官方一等公民配置交换子系统，吸收自 Bun 工业级底层架构与通用数据交换协议，全面支持 INI、TOML、JSON5、DotEnv、YAML 的零拷贝解析、动态类型提取与双向格式化。

---

## 一、 模块拓扑与导入

```typescript
import {
    // 1. INI 节区配置
    Ini, IniParser,
    // 2. TOML 文档状态机
    TOML, TomlDoc,
    // 3. JSON5 带注释数据交换
    JSON5, Json5Scanner,
    // 4. DotEnv 环境变量
    DotEnv,
    // 5. YAML 文档
    YAML, YamlDoc,
    // 6. 通用类型安全提取器
    ConfValueParser
} from "cap:conf";
```

---

## 二、 核心组件与 API 指南

### 1. `Ini` / `IniParser`（分层节区 INI 配置）

#### 特性：
- 支持全局顶层属性（Global properties）与嵌套/点分节区（`[server.database]`）；
- 强大的类型提取器：`getInt`、`getFloat`、`getBool`（全量覆盖 `true`/`1`/`yes`/`on` 与 `false`/`0`/`no`/`off`）；
- 自动文本转义（`\n`, `\r`, `\t`, `\"`）与 `${VAR}` 环境变量插值；
- 动态属性变异与标准输出（`set`, `remove`, `toString()`, `encode()`）。

#### 代码示例：
```typescript
const ini_text = """
app_name = "cp_gateway"
debug = on

[database]
host = "127.0.0.1"
port = 5432
timeout = 30.5
""";

let conf = Ini.parse(ini_text);
let name = conf.get("", "app_name") ?? "default";
let debug = conf.getBool("", "debug", false); // true
let port = conf.getInt("database", "port", 3306); // 5432

conf.set("database", "pool_size", "20");
let serialized = conf.toString(); // 序列化写回规范 INI 文件
```

---

### 2. `TOML` / `TomlDoc`（工业级 TOML 文档解析）

#### 特性：
- 全要素数值：10/16/8/2 进制整数，忽略下划线（`1_000_000`），标准浮点数与科学计数法，`inf`/`nan`；
- 多形态字符串：单行字面量、转义字符串，多行基础字符串 `"""..."""` 与多行字面量 `'''...'''`；
- 点分表与嵌套表（`[database.connection]`）、键级隐式建表（`a.b.c = "v"`）与表数组（`[[array]]`）；
- 数组智能保护提取（`getArray(table, key)`）与标准序列化写回。

#### 代码示例：
```typescript
const toml_text = """
title = "TOML Demo"

[servers.alpha]
ip = "10.0.0.1"
role = "frontend"
ports = [ 80, 443 ]
""";

let doc = TOML.parse(toml_text);
let ip = doc.get("servers.alpha", "ip") ?? "";
let ports = doc.getArray("servers.alpha", "ports"); // ["80", "443"]
let out = doc.toString();
```

---

### 3. `JSON5`（宽容带注释 JSON）

- 完整支持单行/多行注释（`//`, `/* */`）、对象与数组尾随逗号；
- 支持 ECMAScript 未加引号标识符键、十六进制数字（`0x2A`）、显式正负号与 `NaN`/`Infinity`；
- 详细 API 见 [docs/stdlib/json5.md](file:///data/dev/cp/docs/stdlib/json5.md)。

---

## 三、 测试与质量验证

完整测试位于 [cap/test/test_conf_complete.cp](file:///data/dev/cp/cap/test/test_conf_complete.cp)，覆盖率 100%，2/2 PASS。
存量测试 [cap/test/test_config.cp](file:///data/dev/cp/cap/test/test_config.cp) 2/2 全部通过。
