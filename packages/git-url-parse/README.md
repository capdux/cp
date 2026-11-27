[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# git-url-parse

Zero-allocation Git repository URL parser and normalizer for CP — clean-room MIT parity with Rust `git-url-parse` and npm `hosted-git-info`.

Parses SCP-style SSH (`git@github.com:owner/repo.git`), HTTPS/HTTP/SSH+port (`ssh://git@host:2222/...`), `git+https://`, hosted shorthands (`github:`, `gitlab:`, `bitbucket:`, `gitee:`, `codeberg:`, and bare `owner/repo`), `#ref`, browser `/tree/<ref>/<subpath>`, and GitLab nested subgroups into zero-copy slices, with canonical HTTPS, SSH, and Browse URL formatters.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Package | `git-url-parse` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/tjtelan/git-url-parse-rs` |
| License | MIT |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust (crates.io: git-url-parse)
use git_url_parse::GitUrl;
let parsed = GitUrl::parse("git@github.com:capdux/cap.git").unwrap();
```

```cp
// CP (cap: git-url-parse) — zero-copy parsing & stack-buffered normalization
import { parseGitUrl, toHttpsUrl } from "git-url-parse";

const parsed = parseGitUrl("git@github.com:capdux/cap.git");
let buf: [256]u8 = undefined;
const https_url = toHttpsUrl(parsed, buf[0..256]);
```

## Installation

```bash
cap add git-url-parse
```

## Usage

### Basic Usage: Parse & Normalize Between HTTPS, SSH and Browse URLs

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

### Edge Cases: Shorthands, Custom SSH Ports, Nested GitLab Groups & Invalid URLs

```cp
import { parseGitUrl, toSshUrl } from "git-url-parse";

fn demoEdgeCases() {
    // 1. Hosted shorthand & bare "owner/repo"
    const s1 = parseGitUrl("gitlab:org/sub/team/service#main");
    // s1.owner == "org/sub/team", s1.repo == "service", s1.ref_name == "main"

    // 2. Custom SSH port preserved when normalizing to SSH URL
    const s2 = parseGitUrl("ssh://git@git.example.com:2222/team/core.git");
    let buf: [256]u8 = undefined;
    const ssh = toSshUrl(s2, buf[0..256]);
    // => "ssh://git@git.example.com:2222/team/core.git"

    // 3. Whitespace or malformed inputs safely return valid == false
    const bad = parseGitUrl("not a valid url");
    // bad.valid == false; toSshUrl(bad, buf[0..256]) == ""
    _ = s1;
    _ = ssh;
    _ = bad;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `GitProtocol` | `enum { Https, Http, Ssh, Git, File, Shorthand }` | Detected transport protocol. |
| `GitHostKind` | `enum { GitHub, GitLab, Bitbucket, Gitee, Codeberg, Other }` | Detected hosting provider. |
| `ParsedGitUrl` | `class { valid, protocol, host_kind, user, host, port, owner, repo, ref_name, subpath }` | Zero-copy parsed fields (`.git` suffix stripped from `repo`). |
| `parseGitUrl` | `fn(raw: string): ParsedGitUrl` | Parses SCP SSH, HTTPS/HTTP/SSH/Git/File URLs, and hosted shorthands. |
| `toHttpsUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | Formats canonical HTTPS clone URL into `out` (`""` if invalid). |
| `toSshUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | Formats canonical SSH clone URL (SCP or `ssh://` when `port != 0`). |
| `toBrowseUrl` | `fn(parsed: ParsedGitUrl, out: []u8): string` | Formats web browse URL (including `/tree/<ref>/<subpath>` when present). |
| `detectHostKind` | `fn(host: string): GitHostKind` | Classifies a hostname into `GitHostKind`. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `trim`, `eqIgnoreCase`, `findByte`, `rfindByte`), `cap:bin` (`indexOfBytes`), and `cap:fmt` (`writeStr`, `writeU64`, `parseU16`) for fast delimiter scanning and formatting.
- **Zero Heap Allocation & Zero-Copy Slices**: Every field in `ParsedGitUrl` borrows slices directly from `raw` or static literals.
- **Zero-Clear Caller Buffers (`undefined`)**: Normalization writes directly into caller `out: []u8` (declarable with `undefined`) with strict bounds checks.
- **Safe Rejection**: Rejects inputs with embedded whitespace, invalid ports, or missing owner/repo segments without panicking.

## License

MIT — Clean-room CP implementation inspired by crates.io `git-url-parse`.
