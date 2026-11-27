[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# regex-lite

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`crates.io:regex-lite`](https://github.com/rust-lang/regex)（MIT，v0.1 语义）| CP 包版本：`1.0.0` | 依赖：`regex-syntax`、`memchr`

零堆分配、严格线性时间的 Thompson NFA（PikeVM）正则引擎。组合复用 `regex-syntax`（语法校验与字面量前缀提取）与 `memchr`（`memmemFind` 极速跳扫），天然免疫回溯爆炸（ReDoS）。

## 零成本迁移对照（Rust → CP）

直接沿用原包名 `"regex-lite"`，无需改名即可完成迁移：

| Rust (`crates.io:regex-lite`) | CP (`regex-lite`) |
| :--- | :--- |
| `use regex_lite::Regex;` | `import { RegexLite, RegexMatch } from "regex-lite";` |
| `let re = Regex::new(r"\d{4}-\d{2}-\d{2}")?;` | `const re = RegexLite.compile("\\d{4}-\\d{2}-\\d{2}");` |
| `re.is_match(text)` | `re.isMatch(text)` |
| `re.find(text)` | `re.find(text)` |
| `re.find_iter(text)` | `re.findAll(text, out_matches)` |
| `re.replace_all(text, rep)` | `re.replaceAll(text, rep, out)` |

## 核心 API

### 导出常量

- `RL_MAX_INSTS: usize = 128`（最大 NFA 指令数；另内置最多 16 个字符类与 64 字节字面量前缀缓存）

### 类型与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `RegexMatch { matched: bool, start: usize, end: usize }` | 半开字节区间 `[start, end)`，提供 `len(): usize`。 |
| `RegexLite.compile(pattern: string): RegexLite` | 校验并将正则表达式编译为栈上 Thompson NFA。 |
| `re.isOk(): bool` / `re.errCode(): i32` | 检查编译是否成功及错误码（`0` 表示成功）。 |
| `re.isMatch(text: string): bool` | 判断 `text` 中是否存在匹配。 |
| `re.find(text: string): RegexMatch` | 查找 `text` 中最左侧匹配。 |
| `re.findAt(text: string, start_pos: usize): RegexMatch` | 从 `start_pos` 起查找首个匹配。 |
| `re.matchAnchored(text: string, start_pos: usize): RegexMatch` | 仅在 `start_pos` 处执行锚定匹配。 |
| `re.findAll(text: string, out_matches: []RegexMatch): usize` | 收集所有非重叠匹配写入 `out_matches`，返回命中数。 |
| `re.replaceAll(text: string, replacement: string, out: []u8): usize` | 替换全部非重叠匹配写入 `out`，返回写入字节数。 |
| `re.splitCount(text: string): usize` | 统计按匹配切分 `text` 后的分段数量。 |

## 实战示例

### 基础查找、`findAll` 与 `replaceAll`

```cp
import { RegexLite, RegexMatch } from "regex-lite";

fn demoSearchReplace() {
    const re = RegexLite.compile("[a-z]+=[0-9]+");
    if (re.isOk()) {
        const text = "a=10, b=20, c=30";
        const first = re.find(text);
        // first.matched == true, text[first.start..first.end] == "a=10"

        let matches: [8]RegexMatch = undefined;
        const count = re.findAll(text, matches[0..]);
        // count == 3

        let out: [64]u8 = undefined;
        const n = re.replaceAll(text, "X", out[0..]);
        // out[0..n] == "X, X, X"
        * = first; * = count; * = n;
    }
}
```

### 词边界锚点与线性时间防护

```cp
import { RegexLite } from "regex-lite";

fn demoAdvanced() {
    const word_re = RegexLite.compile("\\bfoo\\b");
    const hit1 = word_re.isMatch("bar foo baz"); // true
    const hit2 = word_re.isMatch("foobar");      // false

    // 传统回溯引擎会指数级卡死的模式，在 Thompson NFA 下严格 O(n * m) 完成
    const redos_safe = RegexLite.compile("(a+)+b");
    const hit3 = redos_safe.isMatch("aaaaaaaaaaaaaaaaaaaa!"); // 线性时间返回 false
    * = hit1; * = hit2; * = hit3;
}
```

## 性能与安全亮点

- **严格 O(n × m) Thompson NFA**：基于 `[128]RlInst` 的锁步 PikeVM 执行，彻底杜绝指数级回溯（ReDoS）。
- **SIMD 字面量前缀跳扫**：编译期通过 `regex-syntax::extractLiteralPrefix` 提取前缀，匹配时调用 `memchr::memmemFind` 快速跳转候选位置。
- **零堆分配与 `undefined` 状态数组**：活跃状态表（`cur_list`/`next_list`/`stack: [128]usize = undefined`）与 256 位字符类位图（`[4]u64`）全驻留栈上，无动态分配。

## 许可协议

MIT
