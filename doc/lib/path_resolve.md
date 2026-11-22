# `cap:path_resolve`

> 参考来源：/data/dev/bun/src/paths/resolve_path.cp | 吸收日期：2026-11-22

纯 CP 零堆分配 POSIX 路径规范化、拼接、相对路径、最长公共目录前缀与防穿越校验库。

## 导出符号

- `ParentRelation`：`parent | equal | unrelated`
- `isParentOrEqual(parent, child)` / `isPathInside(parent, child)`（安全防 `..` 目录穿越）
- `longestCommonPath(paths)`：计算多路径最长公共目录前缀
- `normalizePosix(input, out)` / `joinNormalized(base, rel, out)` / `relativePosix(from, to, out)`

## 上游行为契约与目录安全边界

1. **防前缀伪匹配与目录穿越 (`isPathInside` & `isParentOrEqual`)**：
   - 严格按路径段边界判定父子关系（`/var/app` 绝不会误判为 `/var/app2/secret` 的父目录）；结合 `joinNormalized` 先消解 `..` 与冗余 `//` 再执行 `isPathInside`，有效阻断静态文件服务与解压器的路径穿越漏洞。
2. **绝对路径覆写语义 (`joinNormalized`)**：
   - 若 `rel` 本身以 `/` 开头，`joinNormalized` 按 POSIX 规范处理并归一化；输出缓冲 `out` 空间不足时安全返回 `null` 避免静默截断。
3. **最长公共目录前缀 (`longestCommonPath`)**：
   - 始终对齐至完整目录段边界（不会截断目录名中间字符），适合打包器模块根目录推导与 monorepo 工作区定位。
