[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# dotenv-expand

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:dotenv-expand`](https://github.com/motdotla/dotenv-expand)（BSD-2-Clause）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 `.env` 解析器与 POSIX 环境变量展开引擎。支持 `export KEY=VAL`、单/双引号值、`#` 注释、`$VAR`、`${VAR}`、`${VAR:-default}`、`${VAR-default}` 与转义 `\$` 字面量。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"dotenv-expand"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:dotenv-expand`) | CP (`dotenv-expand`) |
| :--- | :--- |
| `import dotenv from "dotenv"; import { expand } from "dotenv-expand";` | `import { parseDotenv, expandEnvValue, EnvMap } from "dotenv-expand";` |
| `const parsed = dotenv.parse(src);` | `const env = parseDotenv(src);` |
| `expand({ parsed })` | `expandEnvValue(template, env, out)` |
| `parsed["API_URL"]` | `env.get("API_URL")` |

## 核心 API

### 导出常量

- `DE_MAX_VARS: usize = 32`

### 类型与函数

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `EnvEntry { key: string, value: string }` | 借用自 `.env` 源文本的键值切片。 |
| `EnvMap.new(): EnvMap` | 创建 32 槽位栈上 `EnvMap`。 |
| `env.len(): usize` / `env.has(key: string): bool` | 查询变量数或键是否存在。 |
| `env.set(*_, key: string, val: string): bool` | 写入或覆盖变量（满 32 条时返回 `false`）。 |
| `env.get(key: string): string?` | 按键读取值（不存在返回 `null`）。 |
| `parseDotenv(src: string): EnvMap` | 解析 `.env` 文本（支持 `export `、引号、`#` 注释、CRLF）。 |
| `expandEnvValue(template: string, env: EnvMap, out: []u8): usize` | 将模板中的 POSIX 变量展开并写入 `out`，返回写入字节数。 |

## 实战示例

### 解析 `.env` 并展开变量

```cp
import { parseDotenv, expandEnvValue } from "dotenv-expand";

fn demoExpand() {
    const dotenv_src = "HOST=api.example.com\nexport PORT=8443\n# comment\nBASE=\"https://${HOST}:${PORT}\"\n";
    const env = parseDotenv(dotenv_src);

    const base_tpl = env.get("BASE").?;
    let out: [128]u8 = undefined;
    const n = expandEnvValue(base_tpl, env, out[0..]);
    // out[0..n] == "https://api.example.com:8443"
    * = n;
}
```

### 默认值回退与 `\$` 转义

```cp
import { EnvMap, expandEnvValue } from "dotenv-expand";

fn demoDefaultsAndEscapes() {
    let env = EnvMap.new();
    * = env.set("EMPTY", "");
    * = env.set("USER", "cp");

    let out: [128]u8 = undefined;
    // ${VAR:-default} 在未定义或为空串时触发默认值
    const n1 = expandEnvValue("${EMPTY:-fallback}", env, out[0..]);
    // out[0..n1] == "fallback"

    // ${VAR-default} 仅在未定义时触发默认值
    const n2 = expandEnvValue("${EMPTY-fallback}", env, out[0..]);
    // out[0..n2] == ""

    // \$ 输出字面量 '$'
    const n3 = expandEnvValue("price=\\$100 user=$USER", env, out[0..]);
    // out[0..n3] == "price=$100 user=cp"
    * = n1; * = n2; * = n3;
}
```

## 性能与安全亮点

- **零堆分配**：`EnvMap` 内联 `[32]EnvEntry = undefined`，初始化零清零且全程无堆分配。
- **`@memcpy` 极速展开**：字面量片段与变量值直接通过 `@memcpy` 写入 `out: []u8`，内置严格长度截断防护。
- **零拷贝解析**：`parseDotenv` 直接返回指向源文本的切片，无临时字符串分配。

## 许可协议

MIT
