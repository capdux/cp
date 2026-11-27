[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# globset

面向 CP 的零堆分配多模式 Glob 集合匹配器（`GlobSet`）与白名单/忽略覆盖规则引擎（`GlobOverride`），对齐 Rust `globset` v0.4 与 `ignore::overrides`。

底层复用 `picomatch`（`scanBase` + `isMatchWithOptions`）与 `normalize-path`，单栈结构支持最多 32 条 Glob 规则，内置静态前缀剪枝加速与 `!` 反向忽略“后命中优先”语义。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `globset` v0.4（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/BurntSushi/ripgrep` |
| 开源协议 | MIT |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: globset)
let mut builder = GlobSetBuilder::new();
builder.add(Glob::new("src/**/*.rs")?);
builder.add(Glob::new("*.toml")?);
let set = builder.build()?;
let matched = set.is_match("src/main.rs");
```

```cp
// CP (cap: globset) — 零堆分配多模式 Glob 集合 + 覆盖规则引擎
import { GlobSet, GlobOverride, GlobSetOptions } from "globset";
let set = GlobSet.new();
set.add("src/**/*.cp");
set.add("*.json");
const matched = set.isMatch("src/main.cp");
```

## 安装

```bash
cap add globset
```

## 实战示例

### 基础用法：多模式匹配与批量收集所有命中索引

```cp
import { GlobSet, GlobSetOptions } from "globset";

let set = GlobSet.new();
set.add("src/**/*.cp");
set.add("**/*.md");
set.add("src/core/*");

// 布尔判断与首个命中索引（未命中返回 -1）
const ok = set.isMatch("src/core/app.cp");       // true
const first = set.firstMatch("src/core/app.cp"); // 0

// 收集所有命中模式的 0-based 下标到栈数组
let hits: [32]usize = undefined;
const n = set.matches("src/core/app.cp", hits[0..32]); // n == 2, hits = [0, 2]
const total = set.matchCount("src/core/app.cp");       // 2

// 忽略大小写匹配
let opts = GlobSetOptions.defaults();
opts.case_insensitive = true;
let ciSet = GlobSet.withOptions(opts);
ciSet.add("*.README");
const ciHit = ciSet.isMatch("docs/guide.readme"); // true
```

### 进阶与安全用法：类 `.gitignore` 白名单与 `!` 忽略覆盖规则

```cp
import { GlobOverride, OVERRIDE_WHITELIST, OVERRIDE_IGNORE, OVERRIDE_NONE } from "globset";

// 正向规则为白名单，`!` 开头为忽略规则，后命中规则覆盖先命中规则！
let ov = GlobOverride.new();
ov.add("src/**");                // 白名单允许 src/ 下全部文件
ov.add("!src/generated/**");     // 忽略生成目录
ov.add("src/generated/keep.cp"); // 重新放行指定文件

const a = ov.isAllowed("src/main.cp");           // true  (OVERRIDE_WHITELIST)
const b = ov.isAllowed("src/generated/tmp.cp");  // false (OVERRIDE_IGNORE)
const c = ov.isAllowed("src/generated/keep.cp"); // true  (后命中优先)
const d = ov.isAllowed("README.md");             // false (存在白名单时未命中路径默认拒绝)

const status = ov.matched("src/generated/tmp.cp"); // OVERRIDE_IGNORE (2)
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `GlobSetOptions` | `class { case_insensitive, literal_separator, backslash_escape, fn defaults() }` | 匹配选项（默认区分大小写、分隔符敏感、允许反斜杠转义）。 |
| `GlobSet` | `class { new(), withOptions(opts), add(*_, pat): bool, len(), isEmpty(), clear(*_), patternAt(idx) }` | 定容（上限 32 条）多模式集合，带静态前缀预过滤。 |
| `GlobSet.isMatch` | `fn(path: string): bool` | 判断路径是否命中任一模式。 |
| `GlobSet.firstMatch` | `fn(path: string): i32` | 返回首个命中模式的 0-based 索引，未命中返回 `-1`。 |
| `GlobSet.matches` | `fn(path: string, out_indices: []usize): usize` | 将全部命中索引写入 `out_indices`，返回写入条数。 |
| `GlobSet.matchCount` | `fn(path: string): usize` | 统计命中模式总数。 |
| `GlobOverride` | `class { new(), withOptions(opts), add(*_, rule): bool, len(), isEmpty(), clear(*_) }` | 白名单（`pat`）与忽略（`!pat`）覆盖引擎，后命中优先。 |
| `GlobOverride.matched` | `fn(path: string): i32` | 返回 `OVERRIDE_NONE` (`0`) / `OVERRIDE_WHITELIST` (`1`) / `OVERRIDE_IGNORE` (`2`)。 |
| `GlobOverride.isAllowed` | `fn(path: string): bool` | 综合判断是否放行路径（存在白名单时未命中路径返回 `false`）。 |

## 边界与性能

- **静态前缀剪枝**：添加模式时通过 `picomatch.scanBase` 预提取最多 64 字节静态目录前缀，快速跳过不匹配路径。
- **零堆分配**：固定上限 `32` 条模式（`GLOBSET_MAX_PATTERNS`）与 `1024` 字节路径规范化栈缓冲。

## 协议

MIT — 对齐 Rust `globset` v0.4 的纯 CP 净室实现。
