# `cap:path_resolve`

纯 CP 零堆分配 POSIX 路径规范化、拼接、相对路径、最长公共目录前缀与防穿越校验库，吸收自 `/data/dev/bun/src/paths/resolve_path.cp`。

## 导出符号

- `ParentRelation`：`parent | equal | unrelated`
- `isParentOrEqual(parent, child)` / `isPathInside(parent, child)`（安全防 `..` 目录穿越）
- `longestCommonPath(paths)`：计算多路径最长公共目录前缀
- `normalizePosix(input, out)` / `joinNormalized(base, rel, out)` / `relativePosix(from, to, out)`
