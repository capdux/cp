[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# conventional-commits-parser

纯 CP 零堆分配 Conventional Commits 1.0.0 规范解析、Header 格式化与 Semver 版本升级自动推导引擎 —— 洁净室 MIT 实现，对标 npm `conventional-commits-parser` 与 Rust `git-conventional`。

完整支持 `<type>[(<scope>)][!]: <description>`、多行 Body、最多 16 条 `Token: value` / `Token #value` Footer、`BREAKING CHANGE` / `BREAKING-CHANGE` 破坏性变更识别以及批量提交的最高 Semver 级别聚合，全流程零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `conventional-commits-parser`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/conventional-changelog/conventional-changelog` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: conventional-commits-parser)
import { CommitParser } from "conventional-commits-parser";
const parser = new CommitParser();
const commit = parser.parse("feat(parser)!: drop legacy v1 syntax");
```

```cp
// CP (cap: conventional-commits-parser) — 零拷贝解析与 Semver 自动推导
import { parseCommit, recommendedBump } from "conventional-commits-parser";

const commit = parseCommit("feat(parser)!: drop legacy v1 syntax");
const bump = recommendedBump(commit); // SemverBump.Major
```

## 安装

```bash
cap add conventional-commits-parser
```

## 使用示例

### 基础用法：解析提交、读取 Footer 与格式化 Header

```cp
import {
    SemverBump,
    parseCommit,
    recommendedBump,
    formatCommitHeader,
} from "conventional-commits-parser";

fn demoCommit() {
    const msg = "feat(parser)!: drop legacy v1 syntax\n\nMigration details here.\n\nBREAKING CHANGE: remove old macro mode\nRefs #42";
    const c = parseCommit(msg);
    if (c.valid) {
        // c.commit_type == "feat", c.scope == "parser", c.is_breaking == true
        // c.breaking_note == "remove old macro mode"
        const bump = recommendedBump(c); // SemverBump.Major
        const refs = c.getFooter("Refs"); // Some("42")

        let hdr_buf: [128]u8 = undefined;
        const hdr = formatCommitHeader(c, hdr_buf[0..128]);
        // => "feat(parser)!: drop legacy v1 syntax"
        _ = bump;
        _ = refs;
        _ = hdr;
    }
}
```

### 边界用法：批量发布版本聚合、Header 校验与非法提交

```cp
import {
    SemverBump,
    parseCommit,
    isValidHeader,
    recommendedBump,
    highestBump,
} from "conventional-commits-parser";

fn demoRelease() {
    // 1. 聚合多条提交的最高 Semver 升级级别
    let release_bump = SemverBump.None;
    const c1 = parseCommit("chore: update docs");
    const c2 = parseCommit("fix(net): guard empty packet");
    const c3 = parseCommit("feat(http): add streaming response");
    release_bump = highestBump(release_bump, recommendedBump(c1));
    release_bump = highestBump(release_bump, recommendedBump(c2));
    release_bump = highestBump(release_bump, recommendedBump(c3));
    // release_bump == SemverBump.Minor

    // 2. 单行 Header 快速校验与非法报文安全回退
    const ok1 = isValidHeader("fix(core): handle EOF"); // true
    const bad = parseCommit("not a conventional commit");
    // bad.valid == false; recommendedBump(bad) == SemverBump.None
    _ = release_bump;
    _ = ok1;
    _ = bad;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `SemverBump` | `enum { None = 0, Patch = 1, Minor = 2, Major = 3 }` | Semver 升级级别枚举 |
| `CommitFooter` | `class { token: string, value: string, is_breaking: bool }` | 单条 Footer 结构 |
| `ParsedCommit` | `class { valid, commit_type, scope, description, body, is_breaking, breaking_note, footers: [16]CommitFooter, footer_count }` | 零拷贝解析结果，内置 `footerAt(i): CommitFooter?` 与 `getFooter(token): string?` |
| `parseCommit` | `fn(msg: string): ParsedCommit` | 解析完整 Conventional Commits 1.0.0 报文 |
| `isValidHeader` | `fn(header: string): bool` | 校验单行提交 Header 合法性 |
| `recommendedBump` | `fn(commit: ParsedCommit): SemverBump` | 推导升级级别（breaking → `Major`、`feat` → `Minor`、`fix` → `Patch`，其余 `None`） |
| `highestBump` / `bumpRank` | `fn(a, b): SemverBump` / `fn(b): u8` | 聚合多次提交中的最高升级级别 |
| `formatCommitHeader` | `fn(commit: ParsedCommit, out: []u8): string` | 将 `ParsedCommit` 格式化回标准 Header 字符串 |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`eqIgnoreCase`、`isAlpha`、`isAlnum`）与 `cap:fmt`（`writeStr`）加速词法扫描与格式化。
- **零堆分配与固定栈池**：内置 `MAX_FOOTERS = 16` 栈池（`[16]CommitFooter`），全部字段均为原报文的零拷贝切片。
- **`undefined` 零清零优化**：Footer 数组采用 `undefined` 声明，避免对未占用槽位执行多余清零。
- **安全失败与边界保护**：畸形提交绝不 Panic，统一返回 `valid == false`；Header 格式化严格受 `out.len` 保护。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `conventional-commits-parser`。
