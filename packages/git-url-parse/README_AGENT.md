# git-url-parse@1.0.0 — AI Agent Quick Ref

> Zero-alloc Git URL parser & HTTPS/SSH/Browse normalizer | Source: `crates.io:git-url-parse` | License: MIT
> Import: `import { GitProtocol, GitHostKind, ParsedGitUrl, parseGitUrl, toHttpsUrl, toSshUrl, toBrowseUrl } from "git-url-parse";` | Types: `src/index.d.cp`

## Core Types

```cp
export enum GitProtocol { Https = 0, Http = 1, Ssh = 2, Git = 3, File = 4, Shorthand = 5 }
export enum GitHostKind { GitHub = 0, GitLab = 1, Bitbucket = 2, Gitee = 3, Codeberg = 4, Other = 5 }

export class ParsedGitUrl {
    valid: bool,
    protocol: GitProtocol,
    host_kind: GitHostKind,
    user: string, host: string, port: u16,
    owner: string, repo: string, ref_name: string, subpath: string,
    fn invalid(): ParsedGitUrl;
}
```

## Core Functions

- `parseGitUrl(raw: string): ParsedGitUrl` — Parses SCP SSH, HTTPS/HTTP/SSH/Git/File, `git+https://`, shorthands (`github:`, `gitlab:`, `bitbucket:`, `gitee:`, `codeberg:`, bare `owner/repo`), `#ref`, `/tree/<ref>/<subpath>`, and GitLab nested groups.
- `toHttpsUrl(parsed: ParsedGitUrl, out: []u8): string` — Canonical `https://...git` clone URL.
- `toSshUrl(parsed: ParsedGitUrl, out: []u8): string` — Canonical `git@host:owner/repo.git` or `ssh://user@host:port/owner/repo.git`.
- `toBrowseUrl(parsed: ParsedGitUrl, out: []u8): string` — Canonical web URL (with `/tree/<ref>/<subpath>` if present).
- `detectHostKind(host: string): GitHostKind` | `stripDotGit(s: string): string` | `parsePort(s: string): u16?`

## Minimal Usage

```cp
import { parseGitUrl, toSshUrl, toBrowseUrl } from "git-url-parse";

const p = parseGitUrl("git+https://github.com/capdux/cap.git#v1.0.0");
if (p.valid) {
    let b1: [256]u8 = undefined;
    let b2: [256]u8 = undefined;
    const ssh = toSshUrl(p, b1[0..256]);     // "git@github.com:capdux/cap.git"
    const web = toBrowseUrl(p, b2[0..256]);  // "https://github.com/capdux/cap/tree/v1.0.0"
}
```

## Pitfalls & Rules

1. **Always check `p.valid`**: Invalid URLs (including embedded whitespace) yield `valid == false`, and `toHttpsUrl`/`toSshUrl`/`toBrowseUrl` return `""`.
2. **Zero-copy slice lifetime**: String fields in `ParsedGitUrl` slice `raw`; keep `raw` alive while reading `p`.
3. **Buffer aliasing**: `toHttpsUrl`, `toSshUrl`, and `toBrowseUrl` return slices of `out`; use separate buffers if holding multiple results at once.
4. **`.git` is auto-stripped**: `p.repo` never contains a trailing `.git` suffix.
5. **Custom ports**: `p.port == 0` means default port; non-zero `port` forces `ssh://` format in `toSshUrl`.
