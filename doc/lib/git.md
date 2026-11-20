# CP 标准库：Git 底层对象、`.gitignore` 引擎与 `.git` 只读内省 (`cap:git` / `cap.git`)

## 一、 架构总览

`cap:git` 是 CP 语言官方内置的零外部依赖 Git 底层原语库，专为 AI Coding Agent、CI 工具链与包管理器提供 Git 对象 SHA-1 内容寻址哈希与二进制编解码、完整 `.gitignore` 规则匹配引擎以及 `.git` 只读 `HEAD`/引用解析能力。全模块**零 `std.posix`、零 `@import("bun")`**，SHA-1 直接调用 Zig 0.17 `std.crypto.hash.Sha1`，文件读取仅使用 `std.c`，支持 `import { ... } from "cap:git"` 与 `import cap from "cap"; cap.git.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/git/object.cp` | `// Source: cp/lib/git/object.cp (Git Blob/Tree/Commit SHA-1 Hashing & Object Parser)` | `GitError`、`GitObjectKind`、`GitOid`、`GitObject`、`GitTreeEntry`、`GitTreeCodec`、`GitCommit`、`GitCommitCodec` |
| `lib/git/ignore.cp` | `// Source: cp/lib/git/ignore.cp (Gitignore Pattern Engine with Negation & Double-Star)` | `GitIgnore`（`#` 注释、`!` 取反、尾部 `/` 目录限定、前导/内部 `/` 锚定、`**` 跨级与 `*`/`?`/`[...]` 通配） |
| `lib/git/repo.cp` | `// Source: cp/lib/git/repo.cp (Read-Only .git HEAD & Ref Inspector)` | `GitHead`、`GitRepo`（符号引用、松散引用、`packed-refs`、detached HEAD 与 worktree `gitdir:`） |
| `lib/git/index.cp` | `// Source: cp/lib/git/index.cp (Unified Exports for cap:git)` | `cap:git` 与 `cap.git` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. OID、对象哈希与 Tree/Commit 编解码（`object.cp`）

- **`GitError`**：`InvalidObject`、`InvalidOid`、`IoError`、`OutOfMemory`
- **`GitObjectKind`**：`Blob`、`Tree`、`Commit`、`Tag`（提供 `asStr()` 与 `fromString(s)`）
- **`GitOid`**（20 字节 `[20]u8`）：
  - `GitOid.parseHex(str): GitOid ! GitError`：严格解析 40 位十六进制字符串（兼容大小写）
  - `GitOid.isValidHex(str): bool`：无分配校验 40 位十六进制格式
  - `toHex(buf: []u8): string` / `toHexAlloc(alloc): ([]u8) ! GitError`：格式化为 40 位小写十六进制
  - `eql(other: GitOid): bool` / `isZero(): bool`
- **`GitObject`**：
  - `GitObject.hashBlob(data: []const u8): GitOid`：计算 `SHA1("blob <len>\0<data>")`，与 `git hash-object` 逐字节一致
  - `GitObject.hashObject(kind: GitObjectKind, payload: []const u8): GitOid`：通用 Git 对象头拼接与 SHA-1 计算
- **`GitTreeEntry` & `GitTreeCodec`**：
  - `GitTreeEntry`：`mode: u32`、`name: string`、`oid: GitOid`、`isDir(): bool`
  - `GitTreeCodec.encode(alloc, entries): ([]u8) ! GitError`：按 Git 规范排序（目录名虚拟追加 `/` 比较）并编码 `<octal_mode> <name>\0<20B_oid>`
  - `GitTreeCodec.decode(alloc, raw_tree): ([]GitTreeEntry) ! GitError`：零拷贝解析 `name`（借用 `raw_tree`），调用方 `alloc.free(entries)`
- **`GitCommit` & `GitCommitCodec`**：
  - `GitCommit`：`tree: GitOid`、`parents: []GitOid`、`author: string`、`committer: string`、`message: string`、`deinit(*_)`
  - `GitCommitCodec.parse(alloc, raw_commit): GitCommit ! GitError`：解析 Commit 头（含多行 `gpgsig` 续行跳过）与提交消息
  - `GitCommitCodec.encode(alloc, commit): ([]u8) ! GitError`：序列化规范 Commit 载荷

### 2. `.gitignore` 规则匹配引擎（`ignore.cp`）

- **`GitIgnore`**：
  - `GitIgnore.init(alloc): GitIgnore` / `deinit(*_)` / `ruleCount(): usize`
  - `addPatterns(*_, text: string): void ! GitError`：解析 `.gitignore` 文本（支持 LF/CRLF、忽略 `#` 注释与空行、剥离未转义尾随空格）
  - `isIgnored(rel_path: string, is_dir: bool): bool`：逐级检查祖先目录与目标路径，完整支持 `!` 取反（last-match-wins）、尾部 `/` 仅匹配目录、前导 `/` 或含 `/` 锚定根路径、`**` 跨多级目录以及 `*` / `?` / `[a-z]` / `[!a-z]` 通配符

### 3. `.git` 只读仓库内省（`repo.cp`）

- **`GitHead`**：`is_detached: bool`、`branch_name: string`、`oid_hex: string`、`deinit(*_)`
- **`GitRepo`**：
  - `GitRepo.readHead(alloc, repo_root: string): GitHead ! GitError`：读取 `<repo_root>/.git/HEAD`（兼容 worktree `.git` `gitdir:` 文件），解析符号引用 `ref: refs/heads/<branch>`（优先松散引用文件，回退检索 `packed-refs`）或 detached 40 位十六进制 OID

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { GitObject, GitOid, GitIgnore, GitRepo } from "cap:git";

pub fn demoGit(): !void {
    const alloc = std.heap.page_allocator;

    const blob_oid = GitObject.hashBlob("hello\n");
    let hex_buf: [40]u8 = undefined;
    const hex = blob_oid.toHex(&hex_buf); // "ce013625030ba8dba906f756967f9e9ca394464a"
    _ = hex;

    let gi = GitIgnore.init(alloc);
    defer gi.deinit();
    gi.addPatterns("*.log\n!important.log\nbuild/\n**/*.tmp\n")?;
    const ignored = gi.isIgnored("src/app.log", false); // true
    _ = ignored;

    let head = cap.git.GitRepo.readHead(alloc, "/data/dev/cp")?;
    defer head.deinit();
}
```
