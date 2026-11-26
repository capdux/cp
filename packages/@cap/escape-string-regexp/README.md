# @cap/escape-string-regexp

零堆分配的 CP 原生正则表达式特殊字符转义库，Clean-Room MIT 实现对标 npm 顶流包 `escape-string-regexp` v5（3.5 亿+/周下载）。

## 特性

- 精确转义全部正则元字符 `[|\\{}()[\]^$+*?.]` 及连字符 `-` → `\x2d`（字符集内同样安全）
- `escapedRegexpLen` 精确预计算输出长度、`hasRegexpMeta` 快速扫描、`unescapeStringRegexp` 反向还原
- 零堆分配、单遍扫描

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`sindresorhus/escape-string-regexp`](https://github.com/sindresorhus/escape-string-regexp)。
