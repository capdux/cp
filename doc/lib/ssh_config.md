# `cap:ssh_config` — OpenSSH `~/.ssh/config` 零分配解析与主机匹配库

> 来源：吸收自 `/data/dev/bun/src/ini.zig`（OpenSSH `ssh_config` Parser & Host Resolver）

`cap:ssh_config` 提供纯 CP、零外部依赖、零堆分配的 OpenSSH `ssh_config(5)` 配置文件解析、多模式 `Host` 通配/否定匹配与首现优先生效（first-obtained-value-wins）主机参数解析能力。

## 核心类型与函数

### `SshHostConfig`

```cp
pub struct SshHostConfig {
    hostname: ([]const u8)? = null,
    user: ([]const u8)? = null,
    port: u16? = null,
    identity_file: ([]const u8)? = null,
    proxy_jump: ([]const u8)? = null,
    forward_agent: bool? = null,
    strict_host_key_checking: ([]const u8)? = null,
}
```

### 核心函数

```cp
pub fn matchHostPatterns(patterns: []const u8, host_alias: []const u8): bool
pub fn resolveSshConfig(content: []const u8, host_alias: []const u8): SshHostConfig
pub fn getSshOption(content: []const u8, host_alias: []const u8, key: []const u8): ([]const u8)?
```

- `matchHostPatterns`：遵循 `ssh_config(5)`，支持空格分隔多模式、`*`/`?` 通配与前导 `!` 否定模式（否定命中即一票否决整个 `Host` 块）。
- `resolveSshConfig`：严格遵循 OpenSSH 首现优先生效规则（先出现的配置不被后续 `Host *` 覆盖），支持 `Key Value` 与 `Key=Value`、双引号值剥离、大小写不敏感关键字、`#` 注释与 UTF-8 BOM。
- `getSshOption`：查询任意自定义 SSH 选项（如 `ServerAliveInterval`、`Compression`）的首次生效值。

## 使用示例

```cp
import { resolveSshConfig, getSshOption } from "cap:ssh_config";

const cfg_txt =
    "Host prod-*\n  User deploy\n  Port 2222\nHost *\n  User root\n  Port 22\n  ForwardAgent no\n";

const c = resolveSshConfig(cfg_txt, "prod-api-1");
// c.user.? == "deploy", c.port.? == 2222, c.forward_agent.? == false
```
