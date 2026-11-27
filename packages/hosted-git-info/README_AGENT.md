# hosted-git-info — AI Agent Quick Ref

Zero-heap parser & URL formatter for GitHub, GitLab, and Bitbucket shorthands, SSH, and HTTPS git URLs (npm hosted-git-info equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HostedGitInfo,
    hgiPush,
    hgiStartsWith,
    hgiEndsWith,
    hgiFindChar,
    hgiBuildFromPath,
    fromUrl,
} from "hosted-git-info";
```

## Core Types & Constants

```cp
class HostedGitInfo { host_type: u8, domain: string, user: string, project: string, /* +2 fields */ } // methods: invalid(...) +4
```

## API Signatures

```cp
fn hgiPush(out: []u8, pos: usize, s: string): usize;
fn hgiStartsWith(s: string, prefix: string): bool;
fn hgiEndsWith(s: string, suffix: string): bool;
fn hgiFindChar(s: string, ch: u8): usize?;
fn hgiBuildFromPath(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo;
fn fromUrl(raw: string): HostedGitInfo;
```

## Minimal Usage

```cp
import { hgiPush } from "hosted-git-info";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hgiPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hosted-git-info"`; install via `cap add hosted-git-info`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
