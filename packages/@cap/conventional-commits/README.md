# @cap/conventional-commits

Zero-allocation Conventional Commits 1.0.0 specification parser, header formatter, and Semver bump recommender for CP — clean-room MIT parity with npm `conventional-commits-parser` and Rust `git-conventional`.

零堆分配的 CP Conventional Commits 1.0.0 规范解析与 Semver 版本号自动推导引擎，支持 `<type>[(<scope>)][!]: <description>`、多行 Body、最多 16 条 `Token: value` / `Token #value` Footer、`BREAKING CHANGE` 识别与批量发布版本号聚合（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/conventional-commits
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `SemverBump` | `pub enum SemverBump: u8 { None, Patch, Minor, Major }` | Semver 升级级别枚举 |
| `CommitFooter` | `pub struct { token, value: string, is_breaking: bool }` | 单条 Footer 结构 |
| `ParsedCommit` | `pub struct { valid, commit_type, scope, description, body, is_breaking, breaking_note, footers, footer_count }` | 完整解析结果（零拷贝切片，提供 `footerAt(i)` 与 `getFooter(token)`） |
| `parseCommit` | `(msg: string) -> ParsedCommit` | 解析 Conventional Commits 1.0.0 完整报文 |
| `isValidHeader` | `(header: string) -> bool` | 校验单行 Conventional Commit Header 合法性 |
| `recommendedBump` | `(commit: ParsedCommit) -> SemverBump` | 推导单次提交的 Semver 升级级别 |
| `highestBump` | `(a: SemverBump, b: SemverBump) -> SemverBump` | 聚合多次提交的最高升级级别 |
| `formatCommitHeader` | `(commit: ParsedCommit, out: []u8) -> string` | 将 `ParsedCommit` 格式化回标准 Header |

## Usage / 使用示例

```cp
import {
    SemverBump,
    parseCommit,
    recommendedBump,
    highestBump,
    formatCommitHeader,
} from "@cap/conventional-commits";

const msg = "feat(parser)!: drop legacy v1 syntax\n\nBREAKING CHANGE: remove old macro mode\nRefs #42";
const c = parseCommit(msg);
const bump = recommendedBump(c); // SemverBump.Major
```

## License

MIT
