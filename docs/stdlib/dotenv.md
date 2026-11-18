# `cap:dotenv` — 生产级环境变量配置解析与级联加载标准库

`cap:dotenv` 模块是 CP 语言官方一等公民标准库中的配置加载与环境隔离引擎，全量重构吸收自 Bun 生产级环境变量管理架构（`/data/dev/bun/src/dotenv/`）。本模块提供符合业界标准（POSIX / Node.js / Bun）的 `.env` 配置文件解析器、多模式级联覆盖状态机、带默认值的变量插值引擎以及安全的转义字符处理。

---

## 模块设计理念

1. **工业级语法兼容与健壮性**：
   - 自动识别并剥离行首的 `export ` 与 `export\t` 声明前缀，与 Shell 脚本无缝互通；
   - 完整支持 `#` 注释（独立注释行与行尾行内注释）；
   - 支持多种引号包裹：
     - **双引号 (`"..."`)**：支持转义字符展开（`\n`, `\r`, `\t`, `\"`, `\\`, `\$`）及跨行多行字符串；
     - **单引号 (`'...'`)**：原生纯字面量（Raw String），内容逐字保留，不进行转义和变量替换；
     - **反引号 (`` `...` ``)**：支持原样字符串包裹；
     - **无引号值**：自动剔除尾随空格与行内注释。
2. **变量插值与默认值机制 (`expand`)**：
   - 支持 `${VAR}` 与 `$VAR` 语法，自动在当前已解析的键值表或系统中查找并替换；
   - 支持带后备默认值的语法：`${VAR:-default_value}`，当目标变量未定义或为空时回退至 `default_value`；
   - 支持转义反斜杠：`\${NOT_EXPANDED}` 保留字面量 `${NOT_EXPANDED}`。
3. **同文件后定义覆盖规则 (Later-Key Override)**：
   - 严格遵循 Bun 与 POSIX 惯例：在同一个 `.env` 文件中，若同一个 Key 被多次定义，后定义的键值自动覆盖先定义的键值。
4. **多环境级联加载优先级状态机 (Cascade Loading)**：
   - 按照业界标准严谨的多层覆盖顺序加载配置（从高到低）：
     1. `.env.<mode>.local`（最高优先级，本地针对特定模式的私密覆盖，如开发或测试本地数据库凭证）；
     2. `.env.local`（本地环境覆盖，当 `mode != "test"` 时生效；测试环境下跳过此项以确保 CI/CD 结果确定性）；
     3. `.env.<mode>`（特定模式通用配置，如 `.env.development`、`.env.production`、`.env.test`）；
     4. `.env`（基础默认配置）。
   - 在级联加载过程中，高优先级文件先加载建立键值，低优先级文件不覆盖已存在的键值。

---

## 核心组件与 API 一览

| 符号 | 类型 | 说明 |
| :--- | :--- | :--- |
| `parse(content: []const u8)` | 函数 | 快速解析字符串中的 `.env` 内容，默认激活变量展开，返回 `EnvMap` |
| `parseWithOptions(content, expand)` | 函数 | 带选项解析，可显式开启或关闭变量插值展开 |
| `expandString(out_buf, map_raw, val)` | 函数 | 底层变量展开函数，将输入字符串中的 `${VAR}` 展开至缓冲区 |
| `config(options: DotenvOptions)` | 函数 | 依据配置选项从磁盘加载单个文件或执行完整级联加载 |
| `load(mode: []const u8)` | 函数 | 依据指定环境模式（如 `"development"`, `"production"`）执行级联加载 |
| `EnvMap` | 结构体 | 预分配内存安全键值映射表，提供 `get()`, `getOr()`, `has()`, `put()` 等方法 |
| `EnvParser` | 结构体 | 零动态分配的词法与语法状态机解析器 |
| `DotenvLoader` | 结构体 | 维护多文件加载状态、记录已成功加载的文件列表 |
| `DotenvOptions` | 结构体 | 加载配置选项（包含 `path`, `mode`, `cwd`, `override`, `expand`） |

---

## API 使用示例

### 1. 基础配置解析与注释

```typescript
import { parse } from "cap:dotenv";

const envContent = `
# 数据库配置
export DB_HOST=127.0.0.1
DB_PORT=5432 # 默认 PostgreSQL 端口
EMPTY_FIELD=
`;

let env = parse(envContent);

assert(env.get("DB_HOST").? == "127.0.0.1");
assert(env.get("DB_PORT").? == "5432");
assert(env.get("EMPTY_FIELD").? == "");
assert(env.has("NON_EXISTENT") == false);
assert(env.getOr("NON_EXISTENT", "fallback") == "fallback");
```

---

### 2. 双引号转义与单引号纯字面量

```typescript
import { parse } from "cap:dotenv";

const envContent = `
DOUBLE_ESCAPED="Line 1\\nLine 2\\tWith Tab"
DOUBLE_QUOTES="Nested \\"Quotes\\" Allowed"
SINGLE_RAW='Literal \\n Not Escaped'
`;

let env = parse(envContent);

// 双引号下的转义符被真实解码为换行与制表符
assert(env.get("DOUBLE_ESCAPED").? == "Line 1\nLine 2\tWith Tab");
assert(env.get("DOUBLE_QUOTES").? == "Nested \"Quotes\" Allowed");

// 单引号内容保持原貌
assert(env.get("SINGLE_RAW").? == "Literal \\n Not Escaped");
```

---

### 3. 变量插值展开与默认值 (`${VAR:-default}`)

```typescript
import { parse } from "cap:dotenv";

const envContent = `
USER=admin
HOST=localhost
PORT=8080
DATABASE_URL=postgres://${USER}@${HOST}:${PORT}/main_db
METRICS_PORT=${CUSTOM_METRICS_PORT:-9090}
ESCAPED_VAR=\${DO_NOT_TOUCH}
`;

let env = parse(envContent);

assert(env.get("DATABASE_URL").? == "postgres://admin@localhost:8080/main_db");
assert(env.get("METRICS_PORT").? == "9090"); // 未定义 CUSTOM_METRICS_PORT，回退至 9090
assert(env.get("ESCAPED_VAR").? == "${DO_NOT_TOUCH}");
```

---

### 4. 级联加载器与环境覆盖 (`DotenvLoader`)

```typescript
import { DotenvLoader, config, DotenvOptions } from "cap:dotenv";

// 方式 A：依据模式执行标准级联加载
// 自动按优先级探测：.env.production.local -> .env.local -> .env.production -> .env
let loader = DotenvLoader.init();
loader.loadCascade(".", "production", false, true);

// 方式 B：使用顶级 config API
let cfg = config(DotenvOptions{
    path: "",              // 为空时自动进入级联模式；指定路径时仅加载该文件
    mode: "development",
    cwd: "/data/project",
    override: false,       // false 表示高优先级键值不被后续低优先级文件覆盖
    expand: true,          // 自动展开变量插值
});

let db_url = cfg.getOr("DATABASE_URL", "sqlite::memory:");
```

---

## 最佳实践与安全指引

1. **版本控制保护**：
   - 严禁将 `.env.local` 与 `.env.<mode>.local` 提交至 Git 仓库，应在 `.gitignore` 中将其排除；
   - 提交 `.env.example` 提供给团队成员作为配置模板。
2. **测试确定性**：
   - 当 `mode == "test"` 时，`loadCascade` 会自动跳过 `.env.local`，确保开发者的本地配置不会干扰单元测试与自动化回归套件。
3. **零内存碎片保障**：
   - `EnvMap` 内置 `[32768]u8` 连续字节池（`buffer`），键值存储与转义解码全量就地完成，不依赖高频碎片堆分配。
