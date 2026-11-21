# `cap:git_url_parse` — Git 远程地址零分配解析与协议互转库

> 来源：吸收自 `/data/dev/bun/src/install/repository.zig`（Git Remote URL Parser & Formatter）

`cap:git_url_parse` 提供纯 CP、零外部依赖、零堆分配的 Git 远程地址解析与 HTTPS / SCP 协议互转能力，完整支持 SCP 简写、`ssh://`、`https://`、`http://`、`git://` 与 `file://`。

## 核心类型

### `GitProtocol`

```cp
pub enum GitProtocol: u8 {
    ssh = 0,
    scp = 1,
    https = 2,
    http = 3,
    git = 4,
    file = 5,

    fn asStr(): string
}
```

### `GitRemoteUrl`

```cp
pub struct GitRemoteUrl {
    protocol: GitProtocol = .https,
    user: ([]const u8)? = null,
    host: ([]const u8)? = null,
    port: u16? = null,
    path: []const u8 = "",      // 不含前导 '/' 与尾随 '.git'
    owner: ([]const u8)? = null,
    repo: []const u8 = "",      // 末段仓库名（不含 '.git'）

    static fn parse(raw: []const u8): GitRemoteUrl?
    fn formatHttps(out: []u8): ([]const u8)?
    fn formatScp(out: []u8): ([]const u8)?
}
```

- `GitRemoteUrl.parse`：支持 `git@github.com:org/repo.git`、`ssh://git@host:2222/a/b/repo.git`、`https://token@github.com/org/repo`、`git://host/org/repo.git`、`file:///var/repos/app.git`。
- `formatHttps`：规范化输出 `https://<host>/<path>.git`（无 host 的 `file://` 返回 `null`）。
- `formatScp`：规范化输出 `<user>@<host>:<path>.git`（从 HTTPS/HTTP 转换时自动使用 `"git"` 用户以避免泄露 token）。

## 使用示例

```cp
import { GitRemoteUrl } from "cap:git_url_parse";

const u = GitRemoteUrl.parse("git@github.com:cp-lang/cp.git").?;
let buf: [128]u8 = [0; 128];
const https_url = u.formatHttps(buf[0..]).?; // "https://github.com/cp-lang/cp.git"
```
