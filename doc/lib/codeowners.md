# `cap:codeowners` — GitHub / GitLab `CODEOWNERS` 解析与归属匹配引擎

> 来源：吸收自 `/data/dev/bun/src/glob.zig`（CODEOWNERS Rule Parser & Owner Resolver）

`cap:codeowners` 提供纯 CP、零外部依赖、零堆分配的 `CODEOWNERS` 规则迭代、Owner 令牌解析与路径代码归属判定能力，完整遵循 GitHub/GitLab 后匹配覆盖语义。

## 核心类型与函数

### `CodeownersRule` & `CodeownersIter`

```cp
pub struct CodeownersRule {
    pattern: []const u8 = "",
    owners_raw: []const u8 = "",
    line_no: usize = 0,
}

pub struct CodeownersIter {
    static fn init(content: []const u8): CodeownersIter
    fn next(*_): CodeownersRule?
}
```

- 自动剥离 UTF-8 BOM，跳过空行、`#` 注释行与 GitLab `[Section]` / `^[Section]` 节头行。
- 支持路径内 `\ ` 转义空格，并自动剥离行尾 `# comment`。

### `OwnerIter`

```cp
pub struct OwnerIter {
    static fn init(owners_raw: []const u8): OwnerIter
    fn next(*_): ([]const u8)?
}
```

- 按空白切分 `@user`、`@org/team`、`a@b.com`，遇到行尾 `#` 注释自动停止。

### `resolveOwners` & `isOwnedBy`

```cp
pub fn resolveOwners(content: []const u8, rel_path: []const u8): ([]const u8)?
pub fn isOwnedBy(content: []const u8, rel_path: []const u8, owner: []const u8): bool
```

- 遵循 GitHub CODEOWNERS 后匹配规则覆盖前规则语义；支持 `*`、`*.cp`、`/docs/`、`docs/*`、`apps/**` 等模式。
- 命中无 owner 的清空规则时返回 `null`。
- `isOwnedBy` 对 owner 名称进行大小写不敏感匹配。

## 使用示例

```cp
import { resolveOwners, isOwnedBy, OwnerIter } from "cap:codeowners";

const co = "* @org/core\n*.cp @alice @bob\n/docs/ @org/docs\n";
const owners = resolveOwners(co, "lib/net/tcp.cp").?; // "@alice @bob"
// isOwnedBy(co, "lib/net/tcp.cp", "@ALICE") == true
```
