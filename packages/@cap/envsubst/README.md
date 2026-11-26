# @cap/envsubst

零堆分配的 POSIX / Docker / Kubernetes 风格环境变量模板展开引擎 — 洁净室 MIT 实现，对标 npm `dotenv-expand` / `envsub` 与 Rust `shellexpand`。

Zero-allocation POSIX / Docker / K8s environment variable template expansion engine for CP — clean-room MIT parity with npm `dotenv-expand`, `envsub`, and Rust `shellexpand`.

## 安装 / Install

```bash
cap add @cap/envsubst
```

## 支持语法 / Supported POSIX Syntax

| 语法 / Syntax | 行为说明 / Behavior |
| --- | --- |
| `$VAR`, `${VAR}` | 基础变量替换（未定义时为空串） |
| `${VAR:-default}` | 未定义或为空串 `""` 时使用 `default`（支持嵌套展开） |
| `${VAR-default}` | 仅当未定义时使用 `default`（已定义为空串 `""` 时保留空串） |
| `${VAR:+alt}` | 已定义且非空时替换为 `alt`，否则为空串 |
| `${VAR+alt}` | 只要已定义（含空串）即替换为 `alt`，否则为空串 |
| `${VAR:?err_msg}` | 未定义或为空串时返回 `ExpandError.MissingRequired` |
| `$$`, `\$` | 字面量 `$` 转义 |

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `EnvMap` | 栈上环境变量表（最多 64 项）：`init()`、`set(*_, key, val)`、`get(key) -> string?`、`has(key) -> bool` |
| `ExpandError` | 错误枚举：`None` / `UnclosedBrace` / `MissingRequired` / `BufferOverflow` |
| `ExpandResult` | 展开结果：`{ ok: bool, err: ExpandError, len: usize, missing_var: string }` |
| `expandEnv` | `(tpl: string, env: EnvMap, out: []u8) -> ExpandResult` |
| `expandEnvStr` | `(tpl: string, env: EnvMap, out: []u8) -> string` |
| `hasEnvVars` | `(tpl: string) -> bool`（忽略 `$$` / `\$` 转义） |

## 使用示例 / Usage

```cp
import { EnvMap, expandEnv, expandEnvStr, hasEnvVars } from "@cap/envsubst";

let env = EnvMap.init();
env.set("HOST", "db.internal");
env.set("PORT", "5432");
env.set("TLS", "1");

let buf: [512]u8 = undefined;
const dsn = expandEnvStr(
    "postgres://${USER:-postgres}@$HOST:${PORT:-5432}/app${TLS:+?sslmode=require}",
    env,
    buf[0..512],
);
// => "postgres://postgres@db.internal:5432/app?sslmode=require"
```

## License

MIT
