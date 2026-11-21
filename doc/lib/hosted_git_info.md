# `cap:hosted_git_info` — 纯 CP 托管 Git 平台 URL 规范化解析与多格式生成器

## 概述

`cap:hosted_git_info` 吸收自 Bun `src/install/hosted_git_info.cp`，纯 CP 零外部依赖、零堆分配，支持 GitHub / GitLab / Bitbucket / Gist 的 shorthand、HTTPS、HTTP、`git+https`、`git+ssh`、SSH SCP 与 `/tree/<branch>` URL 解析，并提供 7 种标准格式 URL 生成。

## 核心类型与 API

- `GitProvider`：`github` / `gitlab` / `bitbucket` / `gist`（提供 `domain()` / `shortcutPrefix()`）
- `HostedGitInfo`：`provider`、`user`、`project`、`committish`、`auth`
  - `HostedGitInfo.parse(input: []const u8): HostedGitInfo?`
  - `toHttps(out)`、`toSsh(out)`、`toSshUrl(out)`、`toShortcut(out)`、`toTarball(out)`、`toBrowse(out)`、`toBugs(out)`

## 示例

```cp
import { HostedGitInfo } from "cap:hosted_git_info";

let buf: [128]u8 = std.mem.zeroes([128]u8);
const info = HostedGitInfo.parse("git@github.com:oven-sh/bun.git#main").?;
const tgz = info.toTarball(buf[0..]).?; // "https://codeload.github.com/oven-sh/bun/tar.gz/main"
```
