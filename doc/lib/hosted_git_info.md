# `cap:hosted_git_info` — 纯 CP 托管 Git 平台 URL 规范化解析与多格式生成器

> 参考来源：/data/dev/bun/src/install/hosted_git_info.cp | 吸收日期：2026-11-22

## 概述

`cap:hosted_git_info` 吸收自 Bun 包管理器核心模块（`src/install/hosted_git_info.cp`），纯 CP 零外部依赖、零堆分配，用于在包管理与依赖解析中将 GitHub / GitLab / Bitbucket / Gist 的各类简写与 URL 规范化解析为结构化信息，并按需生成 7 种标准格式 URL（HTTPS、SSH、SSH URL、Shortcut、Tarball、Browse、Bugs）。

## 上游行为契约与规范细节

- **多协议统一归一（吸收自 Bun `hosted_git_info.cp`）**：
  - 支持以下全部输入形态：
    - 裸简写：`user/repo`、`user/repo#committish`（默认归属 `github`）；
    - 平台简写前缀：`github:user/repo`、`gitlab:user/repo`、`bitbucket:user/repo`、`gist:<id>` 或 `gist:user/<id>`；
    - HTTP/HTTPS 与 `git+https://`、`git+http://`、`git+ssh://`、`git://`、`ssh://` 协议；
    - SCP 风格 SSH：`git@github.com:user/repo.git#main`；
    - 浏览器分支路径：`https://github.com/user/repo/tree/<branch>`（自动提取 `<branch>` 为 `committish`）。
  - 自动剥离尾部 `/` 与大小写不敏感的 `.git` 后缀，识别 `www.github.com` / `www.gitlab.com` / `www.bitbucket.org` 主机别名。
- **Gist 无用户特例**：与 GitHub / GitLab / Bitbucket 强制要求 `user/project` 不同，`gist.github.com` 允许省略 `user`（`user == null`，仅保留 `project` 即 Gist ID），且其 SSH 格式化为 `git@gist.github.com:<id>.git`（不含用户段）。
- **零堆分配格式化**：上游 Bun 使用 `Allocator` 动态 `allocPrint`；CP 标准库全部格式化方法均改为调用方提供栈缓冲 `out: []u8`，容量不足或缺少必需字段时安全返回 `null`。

## 核心类型与 API

### `GitProvider`

```cp
pub enum GitProvider: u8 {
    github = 0,
    gitlab = 1,
    bitbucket = 2,
    gist = 3,

    fn domain(): []const u8
    fn shortcutPrefix(): []const u8
}
```

### `HostedGitInfo`

- 字段：
  - `provider: GitProvider`
  - `user: ([]const u8)?`
  - `project: []const u8`
  - `committish: ([]const u8)?`
  - `auth: ([]const u8)?`
- 静态解析方法：
  - `HostedGitInfo.parse(input: []const u8): HostedGitInfo?`
- 多格式 URL 生成方法（写入 `out: []u8`，返回写入后的切片，缓冲不足返回 `null`）：
  - `toHttps(out: []u8): ([]const u8)?`
  - `toSsh(out: []u8): ([]const u8)?`
  - `toSshUrl(out: []u8): ([]const u8)?`
  - `toShortcut(out: []u8): ([]const u8)?`
  - `toTarball(out: []u8): ([]const u8)?`
  - `toBrowse(out: []u8): ([]const u8)?`
  - `toBugs(out: []u8): ([]const u8)?`

## 边界与安全注意事项

1. **标识符（Slug）合法性校验**：`user` 与 `project` 均拒绝空串及包含空白、`/`:`:`:`@` 的畸形片段，防止路径穿越或 URL 注入。
2. **输出缓冲不足防护**：所有 `to*` 方法在写入前校验 `out.len`，绝不越界写入，不足时返回 `null`。
3. **切片零拷贝生命周期**：解析结果中的 `user`、`project`、`committish`、`auth` 均直接切片借用自 `input`，消费期间需保持原输入存活。

## 示例

```cp
import std from "std";
import { GitProvider, HostedGitInfo } from "cap:hosted_git_info";

let buf: [128]u8 = std.mem.zeroes([128]u8);
if (info := HostedGitInfo.parse("git@github.com:oven-sh/bun.git#main")) {
    // info.provider == GitProvider.github, info.user.? == "oven-sh", info.project == "bun"
    if (tgz := info.toTarball(buf[0..])) {
        // "https://codeload.github.com/oven-sh/bun/tar.gz/main"
        * = tgz;
    }
}
```
