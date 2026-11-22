# `cap:dotenv` — 零分配 `.env` 环境变量解析、变量插值与多环境级联加载器

> 参考来源：`/data/dev/bun/docs/runtime/environment-variables.mdx`, `/data/dev/bun/src/dotenv/env_loader.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/dotenv/`  
> **导入协议**: `import { parse, parseWithOptions, expandString, EnvMap, DotenvOptions, DotenvLoader, config, load } from "cap:dotenv";`

---

## 一、 架构概述与设计亮点

`cap:dotenv` 吸收自 Bun 内置的 `.env` 加载引擎（`bun/src/dotenv/env_loader.cp`），采用**全栈内建线性竞技场缓冲（Zero-Heap In-Struct Arena Buffer）**架构：

1. **零堆分配存储结构 (`EnvMap`)**：内置 `256` 个键值对槽位（`MAX_ENV_ENTRIES`）与 `32KB` 连续字符缓冲区（`ENV_BUFFER_CAPACITY`），所有反转义、CRLF 换行归一化与变量插值后的字符串直接落在结构体内部缓冲区，零系统堆分配。
2. **完备 `.env` 词法与多行引号状态机 (`EnvParser`)**：
   - 自动剥离文件开头 **UTF-8 BOM (`\xEF\xBB\xBF`)**；完整兼容 **CRLF (`\r\n`)**、孤立 `\r`、LF (`\n`) 与 **UTF-8 空白行**（包括空格、Tab、VT/FF、`U+00A0` NBSP 与 `U+3000` 全角空格）。
   - 支持标准 `KEY=VAL`、`KEY: VAL` 与 `export KEY=VAL`（空格或 Tab 分隔）。
   - 支持未加引号值（自动剥离行内 `# 注释`，支持 `KEY=#comment` 置空而保留引号内 `#` 与非空白分隔 URL 锚点）、单引号跨行原始字面量 `'...'`、反引号跨行字面量 `` `...` ``（内部可自由包含 `'` 与 `"`）以及双引号跨行转义字符串 `"..."`（解析 `\n`、`\r`、`\t`、`\"`、`\\` 与 `\$`）。
   - 支持引号闭合后的同行尾 `# 注释`（如 `KEY="val" # inline comment`）。
3. **Shell 风格变量插值与默认值回退引擎 (`expandString`)**：
   - 支持 `$VAR`（按 POSIX 标识符边界 `[a-zA-Z_][a-zA-Z0-9_]*` 精准切分，如 `$HOST:$PORT`、`$USER-dev`）与 `${VAR}`。
   - 严格区分 **`${VAR:-default}`**（变量**未定义或为空字符串**时使用 `default`）与 **`${VAR-default}`**（仅当变量**未定义**时使用 `default`，若已定义为空串则保持空串 `""`）。
   - 支持**嵌套默认值**（如 `${A:-${B:-fallback}}`）与**链式递归引用**（深度上限 8 层防环），以及 `\$` 转义输出字面量 `$`。
4. **Bun 标准四级多环境级联加载 (`DotenvLoader.loadCascade`)**：按优先级从高到低依次加载 `.env.<mode>.local` -> `.env.local` -> `.env.<mode>` -> `.env`。

---

## 二、 `.env` 语法规则与多环境优先级矩阵

### 1. 语法解析规则对照表

| 语法特性 | `.env` 示例 | 解析结果 (`env.get(...)`) | 规则说明 |
| :--- | :--- | :--- | :--- |
| **UTF-8 BOM & CRLF** | `\xEF\xBB\xBFKEY=val\r\n` | `"val"` | 自动剥离 UTF-8 BOM 并归一化 CRLF 换行 |
| **`export` 前缀** | `export NODE_ENV=production` | `"production"` | 自动跳过行首 `export ` / `export\t` |
| **行内 `#` 注释** | `PORT=8080 # 端口` / `E=#note` | `"8080"` / `""` | 仅在非引号内生效；引号内 `#` 原样保留 |
| **双引号多行与转义** | `MSG="a\nb\t\"c\"\\d\n"` | `"a\nb\t\"c\"\\d\n"` | 解析 `\n`, `\r`, `\t`, `\"`, `\\`, `\$`，并支持跨行续行 |
| **单引号/反引号多行** | ``TXT=`a 'b' "c"\nd` `` | `"a 'b' \"c\"\nd"` | 原始保留反斜杠，自动归一化内部 `\r\n` -> `\n` |
| **标识符边界展开** | `URL=http://$HOST:$PORT` | `"http://127.0.0.1:8080"` | `$VAR` 在 `:` / `-` / `.` 边界自动截断 |
| **`:-` vs `-` 回退** | `${EMPTY:-def}` vs `${EMPTY-def}` | `"def"` vs `""` | `:-` 对空串回退；`-` 仅对未定义变量回退 |
| **嵌套默认值展开** | `${A:-${B:-fallback}}` | `"fallback"` | 支持任意合法大括号嵌套插值 |
| **美元符号转义** | `RAW="\${KEEP}"` | `"${KEEP}"` | `\$` 输出字面量 `$` 且阻止二次展开 |

### 2. 多环境级联加载优先级（Cascade Priority）

| 优先级排序 | 文件名模板 | `mode = "development"` 示例 | 适用场景说明 |
| :---: | :--- | :--- | :--- |
| **1 (最高)** | `.env.<mode>.local` | `.env.development.local` | 特定环境下的本地私有覆盖配置 |
| **2** | `.env.local` | `.env.local` | 所有环境通用本地配置（`mode == "test"` 时自动跳过） |
| **3** | `.env.<mode>` | `.env.development` | 特定环境共享配置 |
| **4 (最低)** | `.env` | `.env` | 全局基础默认配置 |

---

## 三、 核心 API 与 `Bun.env` / `dotenv` 对照表

| 功能描述 | `cap:dotenv` API | Node `dotenv` / Bun 对照 | 返回类型 |
| :--- | :--- | :--- | :--- |
| **解析内存 `.env` 文本** | `parse(content)` / `EnvMap.parse(content)` | `dotenv.parse(src)` | `EnvMap` |
| **可控插值解析** | `parseWithOptions(content, expand)` | `dotenvExpand(dotenv.parse(src))` | `EnvMap` |
| **读取环境变量** | `env.get(key)` / `env.getOr(key, def)` | `Bun.env[key] ?? def` | `([]const u8)?` / `[]const u8` |
| **判断变量是否存在** | `env.has(key)` | `key in Bun.env` | `bool` |
| **按选项加载文件/级联** | `config(DotenvOptions{ ... })` | `dotenv.config({ path, override })` | `DotenvLoader` |
| **按环境模式一键级联加载** | `load("production")` | Bun 启动自动加载 `.env.*` | `DotenvLoader` |

---

## 四、 代码示例

```typescript
const std = @import("std");
import { parse, DotenvLoader, DotenvOptions, config } from "cap:dotenv";

fn demoDotenv() {
    const src =
        "\xEF\xBB\xBF" ++ // UTF-8 BOM
        "# 数据库配置\r\n" ++
        "export DB_HOST=127.0.0.1\r\n" ++
        "DB_PORT=5432 # 注释\r\n" ++
        "EMPTY_STR=\r\n" ++
        "WITH_COLON_DASH=${EMPTY_STR:-postgres}\r\n" ++
        "WITH_DASH_ONLY=${EMPTY_STR-postgres}\r\n" ++
        "NESTED=${CUSTOM_DB:-${FALLBACK_DB:-cp_cloud}}\r\n" ++
        "CERT=`-----BEGIN\r\nDATA-----` # 引号后注释\r\n" ++
        "URL=\"postgres://${WITH_COLON_DASH}@$DB_HOST:$DB_PORT/${NESTED}\"\r\n";

    let env = parse(src);
    // env.get("WITH_COLON_DASH").? => "postgres"
    // env.get("WITH_DASH_ONLY").?  => ""
    // env.get("URL").?             => "postgres://postgres@127.0.0.1:5432/cp_cloud"
    // env.get("CERT").?            => "-----BEGIN\nDATA-----"
}
```

---

## 附加零分配子模块

- **`lib/dotenv/escape.cp`**（`.env` 键名校验与安全值序列化）：`isValidEnvKey`、`needsDotenvQuotes`、`formatDotenvValue`、`formatDotenvLine`、`unescapeDotenvDouble`
- **`lib/dotenv/expand.cp`**（POSIX 环境变量模板展开）：`EnvPair`、`lookupEnv`、`expandEnv`、`hasEnvRefs`

---

## 上游 Bun `.env` 行为契约与边界注意事项

1. **进程环境变量高于 `.env` 文件（参考 `/data/dev/bun/docs/runtime/environment-variables.mdx`）**：
   - 默认非覆盖模式下，已存在于系统进程环境中的变量优先级最高，其次按 `.env.<mode>.local` > `.env.local` > `.env.<mode>` > `.env` 级联填充；在 `NODE_ENV=test` 下跳过 `.env.local` 可确保测试环境结果确定可复现。
2. **引号类型与变量插值规则**：
   - 单引号 `'...'` 为纯字面量（**不执行** `$VAR` 展开与 `\n` 转义）；双引号 `"..."` 与无引号值支持 `$VAR` / `${VAR:-default}` / `${VAR-default}` 插值；反引号 `` `...` `` 允许内部同时出现未转义的单双引号。
3. **防循环展开与固定缓冲边界**：
   - 链式变量引用（如 `A=$B; B=$A`）受最大 8 层递归深度保护；`EnvMap` 固定上限 `MAX_ENV_ENTRIES = 256` 键与 `ENV_BUFFER_CAPACITY = 32KB` 字符竞技场，超限写入安全截断而不触发堆分配。
4. **安全回写序列化（`formatDotenvLine`）**：
   - 生成 `.env` 文件时使用 `isValidEnvKey` 校验键名并通过 `needsDotenvQuotes` / `formatDotenvValue` 自动转义换行、引号与 `#`，防止注入伪造环境变量行。
