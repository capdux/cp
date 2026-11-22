# `cap:aho_corasick` — 纯 CP Aho-Corasick 多模式串匹配自动机

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:aho_corasick`（亦支持 `cap.aho_corasick` 动态挂载）实现线性时间 $O(N + Z)$ Aho-Corasick 多模式字典树与失败指针自动机，支持任意多关键字一次性编译、全重叠匹配检索 (`findAllOverlapping`) 与零分配快速命中检测 (`containsAny`)。

## 核心类型与 API

- `Match`: `{ pattern_id: usize, start: usize, end: usize }`
- `AhoCorasick.init(alloc, patterns)` / `deinit(&self, alloc)`
- `findAllOverlapping(self, alloc, text) -> ![]Match`
- `containsAny(self, text) -> bool`

## 示例

```cp
import { AhoCorasick } from "cap:ds";

const pats: [3][]const u8 = [ "he", "she", "hers" ];
let ac = try AhoCorasick.init(alloc, &pats);
defer ac.deinit(alloc);
const matches = try ac.findAllOverlapping(alloc, "ushers");
defer alloc.free(matches);
```
