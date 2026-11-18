# `cap:semver` — 工业级语义化版本 2.0.0 标准库与查询引擎

`cap:semver` 是 CP 语言的基础设施标准库之一，全量对标并吸收自 Bun 原生语义化版本运行时资产（`bun/src/semver/`，涵盖 `Version.cp`, `SemverQuery.cp`, `SemverRange.cp`, `semver.cp`）。该模块提供了对 **Semver 2.0.0 规范**（含第 11 条点分预发布标识符权重梯级判定、构建元数据忽略）的严格实现，以及工业级版本范围状态机（`^`, `~`, `*`, `x`, `-`, `||`）、集合极值查询（`maxSatisfying`, `minSatisfying`）与原子版本增量运算（`increment`）。

---

## 一、 模块结构与谱系映射

所有文件第 1 行均严格打上对应的 Bun 原生源码谱系标记：

| 文件 | 谱系来源 | 核心职能 |
| :--- | :--- | :--- |
| `lib/semver/version.cp` | `bun/src/semver/Version.cp` | 严格 Semver 2.0.0 版本模型 `Version`、偏版本 `PartialVersion`、权重比较器与 `increment` 增量引擎 |
| `lib/semver/query.cp` | `bun/src/semver/SemverQuery.cp` | 单子句多比较器集合 `ComparatorSet`、比较器 `Comparator`、范围语法糖解析器（`^`, `~`, `-`, `x`, `*`） |
| `lib/semver/range.cp` | `bun/src/semver/SemverRange.cp` | 逻辑析取范围 `Range`（多段 `||` 并集比较）、集合极值选择器 `maxSatisfying` 与 `minSatisfying` |
| `lib/semver/index.cp` | `bun/src/semver/semver.cp` | 模块对外统一聚合导出入口及向后兼容 `Semver` 命名空间 |

---

## 二、 核心组件与 API 规范

### 1. 语义化版本实体 (`Version`)
- **字段**：
  - `major: u32`: 主版本号
  - `minor: u32`: 次版本号
  - `patch: u32`: 补丁版本号
  - `prerelease: []const u8`: 预发布版本标签（点分隔标识符）
  - `build: []const u8`: 构建元数据（由 `+` 引导，不参与版本比较）
- **核心方法**：
  - `parse(str: []const u8): !Version`: 解析完整的版本字符串（自动剥离可选的前导 `v`/`V`/`=` 及首尾空白）
  - `parsePartial(str: []const u8): !PartialVersion`: 解析可能包含通配符或省略字段的偏版本（如 `1.x`, `1.2`, `*`）
  - `order(a: Version, b: Version): std.math.Order`: Semver 2.0.0 规范第 11 条判定
  - `compare(a: Version, b: Version): std.math.Order`: 别名方法
  - `eql(a: Version, b: Version): bool`: 等价判断
  - `lt`, `lte`, `gt`, `gte`: 偏序断言方法
  - `format(self: Version, buf: []u8): []const u8`: 规范序列化为文本
  - `increment(self: Version, release_type: ReleaseType, identifier: ([]const u8)?): Version`: 版本增量运算
  - `incrementWithBuf(self: Version, release_type: ReleaseType, identifier: ([]const u8)?, buf: []u8): Version`: 支持外部缓冲区的版本增量

### 2. Semver 2.0.0 第 11 条权重梯级规则
当两个主、次、补丁版本完全相同的版本比较时，依次按以下次序计算优先级：
1. **正式发布版本高于预发布版本**：
   - `1.0.0 > 1.0.0-rc.1`
2. **点分标识符由左至右逐段比对**：
   - 纯数字段按**数值大小**比较：`1.0.0-beta.2 < 1.0.0-beta.11`
   - 含字母或连字符段按 **ASCII 字典序**比较：`1.0.0-alpha < 1.0.0-beta`
   - **数字标识符优先级始终低于非数字标识符**：`1.0.0-1 < 1.0.0-a`，`1.0.0-alpha.1 < 1.0.0-alpha.beta`
   - 较长的前缀完全相同的标识符集合优先级高于较短集合：`1.0.0-alpha < 1.0.0-alpha.1`
3. **构建元数据不影响版本次序**：
   - `1.0.0+20130313 == 1.0.0+exp.sha.5114f85 == 1.0.0`

### 3. 高级范围匹配状态机 (`Query` / `Range`)
支持全套现代包管理器版本范围表达式：
- **脱字符范围 (`^`)**：
  - `^1.2.3` := `>=1.2.3 <2.0.0`
  - `^0.2.3` := `>=0.2.3 <0.3.0`
  - `^0.0.3` := `>=0.0.3 <0.0.4`
- **波浪号范围 (`~`)**：
  - `~1.2.3` := `>=1.2.3 <1.3.0`
  - `~1.2` := `>=1.2.0 <1.3.0`
  - `~1` := `>=1.0.0 <2.0.0`
- **通配符 / X 范围**：
  - `*`, `x`, `X` := 匹配任意版本
  - `1.x` := `>=1.0.0 <2.0.0`
  - `1.2.*` := `>=1.2.0 <1.3.0`
- **连字符范围 (`A - B`)**：
  - `1.2.3 - 2.3.4` := `>=1.2.3 <=2.3.4`
  - `1.2.3 - 2.3` := `>=1.2.3 <2.4.0`
  - `1.2.3 - 2` := `>=1.2.3 <3.0.0`
- **逻辑析取 (`||`)**：
  - `^1.0.0 || ^3.0.0`: 满足任一子句即判定为匹配

### 4. 集合极值选择器
- `maxSatisfying(versions: []const Version, range: []const u8): Version?`: 挑选满足范围的最高版本
- `minSatisfying(versions: []const Version, range: []const u8): Version?`: 挑选满足范围的最低版本
- `maxSatisfyingStr(strs: []const []const u8, range: []const u8): ([]const u8)?`: 字符串数组版本
- `minSatisfyingStr(strs: []const []const u8, range: []const u8): ([]const u8)?`: 字符串数组版本

---

## 三、 代码范例

### 1. 版本解析与比较
```typescript
import { Version } from "cap:semver";

let v1 = try Version.parse("1.0.0-beta.2");
let v2 = try Version.parse("1.0.0-beta.11");

if (v1.lt(v2)) {
    // 2 < 11 数值段比较正确成立
}
```

### 2. 范围满足判断
```typescript
import { Semver } from "cap:semver";

let ok1 = Semver.satisfiesStr("1.5.0", "^1.2.3"); // true
let ok2 = Semver.satisfiesStr("2.0.0", "^1.2.3"); // false
let ok3 = Semver.satisfiesStr("3.2.0", "^1.0.0 || ^3.0.0"); // true
```

### 3. 版本增量运算
```typescript
import { Version, ReleaseType } from "cap:semver";

let v = try Version.parse("1.2.3");

let v_maj = v.increment(ReleaseType.major, null); // 2.0.0
let v_min = v.increment(ReleaseType.minor, null); // 1.3.0
let v_pre = v.increment(ReleaseType.premajor, "alpha"); // 2.0.0-alpha

let v_pre1 = try Version.parse("1.2.3-alpha.1");
let v_pre2 = v_pre1.increment(ReleaseType.prerelease, null); // 1.2.3-alpha.2
```

### 4. 集合极值匹配
```typescript
import { Semver } from "cap:semver";

let versions: [4][]const u8 = [ "1.0.0", "1.2.0", "1.5.2", "2.0.0" ];
let best = Semver.maxSatisfyingStr(versions[0..4], "^1.0.0");
// best.? => "1.5.2"
```

---

## 四、 自动化测试与质量保障

已通过 `cap test` 原生测试引擎完成全量自动化验证：

1. `cap test cap/test/test_semver.cp`（回归套件）：**4/4 PASS**
   - 基础解析与前导字符清理
   - 基础版本对比与预发布判定
   - 格式化与基础范围运算
2. `cap test cap/test/test_semver_complete.cp`（深度综合套件）：**5/5 PASS**
   - 1. Semver 2.0.0 Section 11 precedence ladder & build metadata
   - 2. Advanced range syntaxes (^, ~, x-range, hyphen)
   - 3. Disjunction logical OR (||) multi-clause matching
   - 4. maxSatisfying & minSatisfying version queries
   - 5. Version increment operations (major, minor, patch, pre*)
