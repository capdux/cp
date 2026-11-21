# `cap:case_fold` — 全量 Unicode Case Folding（1→1 / 1→2 / 1→3 码点折叠）

源自 Bun `md/unicode.cp`（md4c 压缩范围二分查找表），纯 CP 零堆分配实现完整 Unicode 大小写折叠（覆盖 ASCII、拉丁、希腊、西里尔、全角、Deseret、连字 `ﬁ`/`ﬃ`、德语 `ß`/`ẞ`→`ss`、欧姆/开尔文/微符等），以及零分配 `caseFoldEqual` 码点序列比较。

## 导出符号

- `FoldInfo`: `{ codepoints: [3]u21, n_codepoints: u2 }`，提供 `slice()`、`isUnchanged(cp)`
- `caseFold(codepoint: u21): FoldInfo`
- `caseFoldEqual(a: []const u21, b: []const u21): bool`
