# @cap/git-url

Zero-allocation Git repository URL parser and normalizer for CP — clean-room MIT parity with npm `hosted-git-info` and `git-url-parse`.

零堆分配的 CP Git 仓库地址解析与规范化引擎，完整支持 SCP SSH、HTTPS/HTTP/SSH+端口、`git+https`、托管平台简写（`github:`/`gitlab:`/`bitbucket:`/`gitee:`/`codeberg:` 及裸 `owner/repo`）、`#ref`、浏览器 `/tree/<ref>/<subpath>`、GitLab 多级 Group 路径以及 HTTPS/SSH/Browse 规范化输出（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/git-url
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `GitProtocol` | `pub enum { Https, Http, Ssh, Git, File, Shorthand }` | Git 传输协议枚举 |
| `GitHostKind` | `pub enum { GitHub, GitLab, Bitbucket, Gitee, Codeberg, Other }` | 托管平台类型枚举 |
| `ParsedGitUrl` | `pub struct { valid, protocol, host_kind, user, host, port, owner, repo, ref_name, subpath }` | 零拷贝解析结果结构体（自动剥离 `.git`） |
| `parseGitUrl` | `(raw: string) -> ParsedGitUrl` | 解析全部常见 Git URL 与简写格式 |
| `toHttpsUrl` | `(parsed: ParsedGitUrl, out: []u8) -> string` | 规范化输出 HTTPS 克隆地址 |
| `toSshUrl` | `(parsed: ParsedGitUrl, out: []u8) -> string` | 规范化输出 SSH 克隆地址（自动处理自定义端口） |
| `toBrowseUrl` | `(parsed: ParsedGitUrl, out: []u8) -> string` | 规范化输出浏览器访问地址（含 `/tree/<ref>`） |

## Usage / 使用示例

```cp
import { parseGitUrl, toHttpsUrl, toSshUrl, toBrowseUrl } from "@cap/git-url";

let buf: [256]u8 = undefined;
const p = parseGitUrl("git+https://github.com/capdux/cap.git#v1.0.0");
const ssh = toSshUrl(p, buf[0..256]);       // "git@github.com:capdux/cap.git"
const web = toBrowseUrl(p, buf[0..256]);    // "https://github.com/capdux/cap/tree/v1.0.0"
```

## License

MIT
