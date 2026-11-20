// Source: cp/doc/lib/aho.md
# `cap:aho` — Aho-Corasick 多模式字符串匹配自动机

纯 CP、零 C 依赖的高性能 Aho-Corasick 多模式匹配自动机，内置 Trie 前缀树、BFS 失败指针（Failure Link）、字典后缀输出链（Dictionary Suffix Link）以及完整 DFA 状态转移表，支持 $O(N)$ 单遍扫描全重叠匹配、`LeftmostFirst` / `LeftmostLongest` 非重叠消歧、零分配短路检测与高效批量敏感词替换。

## 导入方式

```cp
import {
    AhoError,
    MatchKind,
    AhoMatch,
    AhoBuilder,
    AhoCorasick,
} from "cap:aho";

// 或动态命名空间挂载：
import cap from "cap";
// cap.aho.AhoCorasick / cap.aho.AhoBuilder / cap.aho.MatchKind
```

## 核心类型

### `AhoError`
- `EmptyPattern`：尝试添加空模式串（`pattern.len == 0`）
- `NoPatterns`：在未添加任何模式时调用 `build()`
- `PatternNotCompiled`：在未调用 `build()` 编译自动机前执行匹配或替换
- `InvalidOption`：向 `findLeftmost()` 传入 `MatchKind.AllOverlapping`
- `OutOfMemory`：内存分配失败

### `MatchKind`
- `AllOverlapping`：返回全部重叠命中
- `LeftmostFirst`：左起优先；同起点按最先注册的 `pattern_id` 优先
- `LeftmostLongest`：左起最长；同起点按最长模式优先

### `AhoMatch`
- `pattern_id: usize`：0 基模式编号
- `start: usize`：命中起始字节偏移（包含）
- `end: usize`：命中结束字节偏移（不包含）

## 核心 API

### `AhoCorasick`
- `AhoCorasick.init(allocator): AhoCorasick`：创建区分大小写的自动机实例
- `AhoCorasick.initCaseInsensitive(allocator): AhoCorasick`：创建 ASCII 大小写不敏感自动机实例
- `deinit(*_)`：释放全部状态表、输出链与模式元数据
- `addPattern(*_, pattern: []const u8): usize ! AhoError`：注册单个模式并返回分配的 `pattern_id`
- `addPatterns(*_, patterns: []const []const u8): void ! AhoError`：批量注册模式
- `build(*_): void ! AhoError`：执行 BFS 构建失败指针、字典后缀输出链与完整 DFA 转移表
- `findAll(allocator, text: []const u8): ([]AhoMatch) ! AhoError`：$O(N)$ 单遍扫描返回全部重叠命中
- `findLeftmost(allocator, text: []const u8, kind: MatchKind): ([]AhoMatch) ! AhoError`：按 `LeftmostFirst` 或 `LeftmostLongest` 返回非重叠匹配
- `containsAny(text: []const u8): bool`：命中即短路返回，零堆分配
- `replaceAll(allocator, text: []const u8, replacement: []const u8): ([]u8) ! AhoError`：基于 `LeftmostLongest` 非重叠匹配批量替换关键词
- `patternCount(): usize` / `stateCount(): usize`：查询已注册模式数与自动机状态数

### `AhoBuilder`
- `AhoBuilder.init(allocator)` / `AhoBuilder.initCaseInsensitive(allocator)` / `deinit(*_)`
- `addPattern(*_, pattern)` / `addPatterns(*_, patterns)` / `patternCount()`
- `build(*_): AhoCorasick ! AhoError`：基于暂存模式编译并返回独立的 `AhoCorasick` 实例
