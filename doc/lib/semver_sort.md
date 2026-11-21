# cap:semver_sort

> 源码吸收自 `/data/dev/bun/src/install/semver.zig`，提供纯 CP、零外部依赖、零堆分配的 SemVer 2.0.0 严格优先级比较、预发布标签排序与原地排序/极值选择库。

## 导入方式

```cp
import {
    SemverOrder,
    ParsedSemver,
    parseSemverStrict,
    parseSemverLoose,
    compareSemver,
    compareSemverStr,
    sortSemverAsc,
    sortSemverDesc,
    maxSemverIndex,
    minSemverIndex,
} from "cap:semver_sort";
```

## 核心类型与 API

- `SemverOrder`：`less` / `equal` / `greater`
- `ParsedSemver`：`major: u64`、`minor: u64`、`patch: u64`、`prerelease: []const u8`、`build: []const u8`
- `parseSemverStrict(s: []const u8): ParsedSemver?`：严格遵循 SemVer 2.0.0（拒绝前导零、空标识符与非法字符）
- `parseSemverLoose(s: []const u8): ParsedSemver?`：允许首尾空白与可选单个 `v`/`V` 前缀
- `compareSemver(a: ParsedSemver, b: ParsedSemver): SemverOrder`：完整实现 SemVer 2.0.0 §11（正式版 > 预发布版、纯数字标识符按数值比较且小于非数字、忽略 `+build` 元数据）
- `compareSemverStr(a: []const u8, b: []const u8): SemverOrder?`
- `sortSemverAsc(versions: [][]const u8): bool` / `sortSemverDesc(versions: [][]const u8): bool`：原地稳定插入排序，遇非法版本不破坏原数组并返回 `false`
- `maxSemverIndex(versions: []const []const u8): usize?` / `minSemverIndex(versions: []const []const u8): usize?`

## 使用示例

```cp
import { sortSemverAsc, maxSemverIndex } from "cap:semver_sort";

fn example() {
    let list: [3][]const u8 = ["1.0.0", "1.0.0-alpha.1", "1.0.0-beta"];
    * = sortSemverAsc(&list);
    * = maxSemverIndex(&list); // 2 ("1.0.0")
}
```
