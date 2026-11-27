[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# git-url-parse

纯 CP 零堆分配 Git 仓库地址解析与规范化引擎 —— 洁净室 MIT 实现，对标 Rust `git-url-parse` 与 npm `hosted-git-info`。

支持 SCP 风格 SSH（`git@github.com:owner/repo.git`）、HTTPS/HTTP/SSH+自定义端口、`git+https://`、托管平台简写（`github:`、`gitlab:`、`bitbucket:`、`gitee:`、`codeberg:` 及裸 `owner/repo`）、`#ref`、浏览器 `/tree/<ref>/<subpath>` 与 GitLab 多级 Group 路径，零拷贝切分并支持一键规范化为 HTTPS/SSH/Browse 地址。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **crates.io** |
| 原包名 | `git-url-parse`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/tjtelan/git-url-parse-rs` |
| 许可证 | MIT |

从 Rust 迁移至 CP 时无需改名：

```rust
// Rust (crates.io: git-url-parse)
use git_url_parse::GitUrl;
let parsed = GitUrl::parse("git@github.com:capdux/cap.git").unwrap();
```

```cp
// CP (cap: git-url-parse) — 零拷贝解析与栈缓冲规范化输出
import { parseGitUrl, toHttpsUrl } from "git-url-parse";

const parsed = parseGitUrl("git@github.com:capdux/cap.git");
let buf: [256]u8 = undefined;
const https_url = toHttpsUrl(parsed, buf[0..256]);
```

## 安装

```bash
cap add git-url-parse
```

## 使用示例

### 基础用法：解析并互转 HTTPS / SSH / 浏览器地址

```cp
import {
    GitProtocol,
    GitHostKind,
    parseGitUrl,
    toHttpsUrl,
    toSshUrl,
    toBrowseUrl,
} from "git-url-parse";

fn demoGitUrl() {
    const p = parseGitUrl("git+https://github.com/capdux/cap.git#v1.0.0");
    if (p.valid) {
        // p.protocol == GitProtocol.Https, p.host_kind == GitHostKind.GitHub
        // p.owner == "capdux", p.repo == "cap", p.ref_name == "v1.0.0"
        let b1: [256]u8 = undefined;
        let b2: [256]u8 = undefined;
        let b3: [256]u8 = undefined;
        const https_u = toHttpsUrl(p, b1[0..256]);  // "https://github.com/capdux/cap.git"
        const ssh_u   = toSshUrl(p, b2[0..256]);    // "git@github.com:capdux/cap.git"
        const web_u   = toBrowseUrl(p, b3[0..256]); // "https://github.com/capdux/cap/tree/v1.0.0"
        _ = https_u;
        _ = ssh_u;
        _ = web_u;
    }
}
```

### 边界用法：简写、自定义端口、GitLab 多级 Group 与非法地址

```cp
import { parseGitUrl, toSshUrl } from "git-url-parse";

fn demoEdgeCases() {
    // 1. 托管平台简写与 GitLab 多级分组
    const s1 = parseGitUrl("gitlab:org/sub/team/service#main");
    // s1.owner == "org/sub/team", s1.repo == "service", s1.ref_name == "main"

    // 2. 自定义 SSH 端口在规范化输出时自动保留
    const s2 = parseGitUrl("ssh://git@git.example.com:2222/team/core.git");
    let buf: [256]u8 = undefined;
    const ssh = toSshUrl(s2, buf[0..256]);
    // => "ssh://git@git.example.com:2222/team/core.git"

    // 3. 含空白或畸形地址安全返回 valid == false
    const bad = parseGitUrl("not a valid url");
    // bad.valid == false; toSshUrl(bad, buf[0..256]) == ""
    _ = s1;
    _ = ssh;
    _ = bad;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `GitProtocol` | `enum { Https, Http, Ssh, Git, File, Shorthand }` | 传输协议枚举 |
| `GitHostKind` | `enum { GitHub, GitLab, Bitbucket, Gitee, Codeberg, Other }` | 托管平台类型枚举 |
| `ParsedGitUrl` | `class { valid, protocol, host_kind, user, host, port, owner, repo, ref_name, subpath }` | 零拷贝解析结果结构体（`repo` 自动剥离 `.git`） |
| `parseGitUrl` | `fn(raw: string): ParsedGitUrl` | 解析 SCP SSH、HTTPS/HTTP/SSH/Git/File 与简写格式 |
| `toHttpsUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | 规范化输出 HTTPS 克隆地址（非法返回 `""`） |
| `toSshUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | 规范化输出 SSH 克隆地址（有自定义端口时输出 `ssh://`） |
| `toBrowseUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | 规范化输出浏览器访问地址（自动拼接 `/tree/<ref>/<subpath>`） |
| `detectHostKind` | `fn(host: string): GitHostKind` | 根据主机名识别托管平台类型 |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isSpace`、`trim`、`eqIgnoreCase`、`findByte`、`rfindByte`）、`cap:bin`（`indexOfBytes`）与 `cap:fmt`（`writeStr`、`writeU64`、`parseU16`）加速分隔符查找与端口解析。
- **零堆分配与零拷贝切片**：`ParsedGitUrl` 中全部字符串字段直接切片自原始输入 `raw` 或静态字面量。
- **`undefined` 零清零栈缓冲**：规范化输出直接写入调用方栈缓冲 `out: []u8`（可用 `undefined` 声明），全程边界校验。
- **安全拒绝畸形输入**：遇到含空格、非法端口或缺失 owner/repo 段的地址时安全返回 `valid == false`，绝不 Panic。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 crates.io `git-url-parse`。
