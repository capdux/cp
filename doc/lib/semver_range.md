# `cap:semver_range` — NPM / Semver 2.0 版本解析与范围匹配引擎

> 溯源：从 `/data/dev/bun/src/semver/SemverRange.cp` 与 `Version.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:semver_range` 提供零外部依赖、零堆分配的完整 Semver 2.0 版本比较与 NPM 范围匹配引擎：

- **`SemverVersion`**：严格遵循 Semver 2.0 规范（含 prerelease 点分数字/字典序优先级、忽略 build 元数据、拒绝数字前导零）；
- **`SemverRange`**：支持 `^`（含 `^0.2.3`/`^0.0.3` 特殊语义）、`~`、`>=`/`<=`/`>`/`<`/`=`/`==`、连字符 `1.0.0 - 2.3.4`、通配符 `*`/`1.x`/`1.2.*`、`||` 多组逻辑或、NPM Prerelease 保护及 `maxSatisfying` / `minSatisfying` 选版。

---

## 快速导入

```cp
import {
    SemverVersion,
    SemverRange,
    Comparator,
    ComparatorGroup,
    Op,
    Error,
} from "cap:semver_range";
```

---

## API 参考

### 1. `SemverVersion`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` | `static fn parse(input: string): SemverVersion!` | 解析标准 Semver 2.0（支持可选 `v`/`V` 前缀） |
| `hasPrerelease` / `hasBuild` | `fn ...(): bool` | 是否含预发布标签或构建元数据 |
| `compare` | `fn compare(other: SemverVersion): std.math.Order` | Semver 2.0 全序比较（忽略 `build`） |
| `eql` / `lt` / `lte` / `gt` / `gte` | `fn ...(other: SemverVersion): bool` | 关系比较快捷方法 |

### 2. `SemverRange`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` | `static fn parse(input: string): SemverRange!` | 解析 NPM Semver 范围表达式 |
| `groupCount` | `fn groupCount(): usize` | 返回 `||` 分组数量 |
| `satisfies` | `fn satisfies(v: SemverVersion): bool` | 判断版本号是否满足范围 |
| `satisfiesStr` | `fn satisfiesStr(ver_str: string): bool` | 便捷字符串版本匹配 |
| `maxSatisfying` | `fn maxSatisfying(versions: []const SemverVersion): SemverVersion?` | 选出满足范围的最大版本 |
| `minSatisfying` | `fn minSatisfying(versions: []const SemverVersion): SemverVersion?` | 选出满足范围的最小版本 |

---

## 使用示例

```cp
import { SemverRange, SemverVersion } from "cap:semver_range";

fn pickBest()! {
    const range = SemverRange.parse("^1.2.0 || >=3.0.0 <3.2.0")?;
    const candidates: [4]SemverVersion = [
        SemverVersion.parse("1.2.5")?,
        SemverVersion.parse("1.9.0")?,
        SemverVersion.parse("2.0.0")?,
        SemverVersion.parse("3.1.4")?,
    ];
    if (best := range.maxSatisfying(candidates[0..4])) {
        * = best; // 3.1.4
    }
}
```
