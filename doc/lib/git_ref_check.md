# `cap:git_ref_check` — Git 引用名格式校验与分类拆解库

> 来源：吸收自 `/data/dev/bun/src/install/repository.zig`（Git Refname Validator & Classifier）

`cap:git_ref_check` 提供纯 CP、零外部依赖、零堆分配的 `git-check-ref-format` 全套规则校验、分支名校验、完整引用分类拆解（`refs/heads/`、`refs/tags/`、`refs/remotes/`、`HEAD`）及 SHA-1/SHA-256 对象 ID 校验能力。

## 核心类型与函数

### `GitRefKind` & `ParsedGitRef`

```cp
pub enum GitRefKind: u8 {
    branch = 0,
    tag = 1,
    remote = 2,
    symbolic = 3,
    other = 4,

    fn asStr(): string
}

pub struct ParsedGitRef {
    kind: GitRefKind = .other,
    short_name: []const u8 = "",
    remote_name: ([]const u8)? = null,
}
```

### 核心函数

```cp
pub fn isValidGitRefName(ref_name: []const u8): bool
pub fn isValidBranchName(branch: []const u8): bool
pub fn parseFullGitRef(full_ref: []const u8): ParsedGitRef?
pub fn isValidGitOidHex(oid: []const u8): bool
pub fn isValidShortOidHex(oid: []const u8): bool
```

- `isValidGitRefName`：严格遵循 `git-check-ref-format` 全部 10 条规则（禁止控制字符/DEL/空格/`~`/`^`/`:`/`?`/`*`/`[`/`\`、禁止 `..`/`@{`/`//`/单 `@`、禁止首尾 `/` 与尾 `.`、禁止任意段以 `.` 开头或以 `.lock` 结尾）。
- `isValidBranchName`：在 `isValidGitRefName` 基础上禁止以 `-` 开头及保留名 `"HEAD"`。
- `parseFullGitRef`：零拷贝拆解 `refs/heads/<branch>`、`refs/tags/<tag>`、`refs/remotes/<remote>/<branch>` 与符号引用 `HEAD`/`FETCH_HEAD`。
- `isValidGitOidHex` / `isValidShortOidHex`：校验 40 位 SHA-1 / 64 位 SHA-256 完整 OID 及 4..=64 位短 OID。

## 使用示例

```cp
import { parseFullGitRef, isValidBranchName, isValidGitOidHex } from "cap:git_ref_check";

const r = parseFullGitRef("refs/remotes/origin/feat/http2").?;
// r.kind == .remote, r.remote_name.? == "origin", r.short_name == "feat/http2"
```
