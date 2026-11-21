# cap:env_expand

> 源码吸收自 `/data/dev/bun/src/env_loader.zig`，提供纯 CP、零外部依赖、零堆分配的 POSIX Shell 风格环境变量模板展开器。

## 导入方式

```cp
import { EnvPair, lookupEnv, expandEnv, hasEnvRefs } from "cap:env_expand";
```

## 核心类型与 API

### `EnvPair`

- `key: []const u8`
- `value: []const u8`

### 核心函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `lookupEnv` | `fn lookupEnv(pairs: []const EnvPair, key: []const u8): ([]const u8)?` | 查找变量（同名后者覆盖前者） |
| `expandEnv` | `fn expandEnv(template: []const u8, pairs: []const EnvPair, out: []u8): ([]const u8)?` | 支持 `$VAR`、`${VAR}`、`${VAR:-def}`、`${VAR-def}`、`${VAR:+alt}`、`${VAR+alt}`、`$$` 与 `\$` 转义 |
| `hasEnvRefs` | `fn hasEnvRefs(template: []const u8): bool` | 检测是否存在未转义的环境变量引用 |

## 使用示例

```cp
import { EnvPair, expandEnv } from "cap:env_expand";

fn example() {
    const envs: [2]EnvPair = [
        EnvPair{ key: "HOST", value: "127.0.0.1" },
        EnvPair{ key: "PORT", value: "" },
    ];
    let buf: [128]u8 = undefined;
    * = expandEnv("http://$HOST:${PORT:-8080}/api", &envs, &buf); // "http://127.0.0.1:8080/api"
}
```
