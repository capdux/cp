# `cap:dotenv` — 零分配 `.env` 环境变量解析、变量插值与多环境级联加载器

> **上游溯源**: `/data/dev/bun/src/dotenv/` (`env_loader.cp`, `env_loader.zig`)  
> **模块路径**: `lib/dotenv/`  
> **导入协议**: `import { parse, parseWithOptions, expandString, EnvMap, DotenvOptions, DotenvLoader, config, load } from "cap:dotenv";`

---

## 一、 架构概述与设计亮点

`cap:dotenv` 吸收自 Bun 内置的 `.env` 加载引擎（`bun/src/dotenv/env_loader.cp`），采用**全栈内建线性竞技场缓冲（Zero-Heap In-Struct Arena Buffer）**架构：

1. **零堆分配存储结构 (`EnvMap`)**：内置 `256` 个键值对槽位（`MAX_ENV_ENTRIES`）与 `32KB` 连续字符缓冲区（`ENV_BUFFER_CAPACITY`），所有反转义与变量插值后的字符串直接落在结构体内部缓冲区，无需调用系统堆分配器。
2. **完整的 `.env` 词法语法支持 (`EnvParser`)**：
   - 支持标准 `KEY=VALUE`、`KEY: VALUE` 以及 Shell 风格的 `export KEY=VALUE` 声明。
   - 支持未加引号值（自动剥离行尾 `#` 内联注释）、单引号原始字面量 `'raw'`、反引号字面量 `` `raw` `` 以及双引号转义字符串 `"line1\nline2\t\"quoted\""`。
3. **Shell 风格变量插值与默认值引擎 (`expandString`)**：
   - 支持 `$VAR` 与 `${VAR}` 引用已定义的环境变量。
   - 支持 `${VAR:-default_value}` 缺省回退语法（当 `VAR` 未定义或为空字符串时自动采用 `default_value`）。
   - 支持 `\$` 转义字面量美元符号，阻止变量展开。
4. **Bun 标准四级多环境级联加载 (`DotenvLoader.loadCascade`)**：按优先级从高到低依次加载 `.env.<mode>.local` -> `.env.local` -> `.env.<mode>` -> `.env`。

---

## 二、 `.env` 语法规则与多环境优先级矩阵

### 1. 语法解析规则对照表

| 语法特性 | `.env` 示例 | 解析结果 (`env.get(...)`) | 规则说明 |
| :--- | :--- | :--- | :--- |
| **基础键值对** | `PORT = 8080` | `"8080"` | 自动修剪键名与值两端的空白字符 |
| **`export` 前缀** | `export NODE_ENV=production` | `"production"` | 自动识别并跳过行首 `export ` 或 `export\t` |
| **冒号分隔符** | `HOST: 127.0.0.1` | `"127.0.0.1"` | 兼容 YAML 风格 `:` 键值分隔符 |
| **行尾内联注释** | `TIMEOUT=30 # 单位秒` | `"30"` | 未加引号值中，空格后紧跟的 `#` 视为内联注释 |
| **双引号转义** | `MSG="Hello\nWorld\t\"CP\""` | `"Hello\nWorld\t\"CP\""` | 解析 `\n`, `\r`, `\t`, `\"`, `\\`, `\$` 转义序列 |
| **单引号原样字面量** | `RAW='literal \n texto'` | `"literal \\n texto"` | 单引号内不执行反斜杠转义，原样保留 |
| **变量插值展开** | `URL=http://${HOST}:${PORT}` | `"http://127.0.0.1:8080"` | 自动引用前文或同文件内定义的变量 |
| **默认值回退 (`:-`)** | `POOL=${DB_POOL:-16}` | `"16"` | 当 `DB_POOL` 未定义或为空时回退至 `"16"` |
| **美元符号转义** | `PRICE=\${AMOUNT}` | `"${AMOUNT}"` | `\$` 输出字面量 `$` 且不触发插值 |

### 2. 多环境级联加载优先级（Cascade Priority）

当调用 `load(mode)` 或 `config(DotenvOptions{ mode: "development", override: false })` 时，加载器按以下优先级顺序读取（先加载的高优变量不会被后加载的低优文件覆盖）：

| 优先级排序 | 文件名模板 | `mode = "development"` 示例 | 适用场景说明 |
| :---: | :--- | :--- | :--- |
| **1 (最高)** | `.env.<mode>.local` | `.env.development.local` | 特定环境下的本地私有覆盖配置（通常加入 `.gitignore`） |
| **2** | `.env.local` | `.env.local` | 所有环境通用的本地私有配置（注：`mode == "test"` 时自动跳过以保证测试确定性） |
| **3** | `.env.<mode>` | `.env.development` | 特定环境（`development` / `production` / `test`）团队共享配置 |
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

### 1. 解析 `.env` 字符串与 `${VAR:-default}` 插值展开

```typescript
const std = @import("std");
import { parse } from "cap:dotenv";

fn demoDotenvParse(): void {
    const src =
        "# 数据库配置\n" +
        "DB_HOST=127.0.0.1\n" +
        "DB_PORT=5432\n" +
        "export DB_NAME=cp_cloud\n" +
        "DB_USER=${CUSTOM_USER:-postgres}\n" +
        "DATABASE_URL=\"postgres://${DB_USER}@${DB_HOST}:${DB_PORT}/${DB_NAME}\"\n";

    let env = parse(src);

    let url = env.get("DATABASE_URL").?;
    // url => "postgres://postgres@127.0.0.1:5432/cp_cloud"

    let timeout = env.getOr("CONNECT_TIMEOUT", "10s");
    // timeout => "10s"
}
```

### 2. 多环境级联加载器 (`DotenvLoader`)

```typescript
import { DotenvLoader, DotenvOptions, config } from "cap:dotenv";

fn demoCascadeLoader(): void {
    // 1. 从工作目录自动按 .env.production.local -> .env.local -> .env.production -> .env 级联加载
    let prod_loader = config(DotenvOptions{
        path: "",
        mode: "production",
        cwd: ".",
        override: false,
        expand: true,
    });

    // 2. 手动模拟多层级联覆盖（overwrite = false 保留先加载的高优先级键）
    let custom = DotenvLoader.init();
    custom.loadFromString("PORT=9000\n", false, true); // 高优：.env.local
    custom.loadFromString("PORT=3000\nHOST=0.0.0.0\n", false, true); // 低优：.env

    // custom.get("PORT").? => "9000"
    // custom.get("HOST").? => "0.0.0.0"
}
```
