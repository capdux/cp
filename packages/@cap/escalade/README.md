# @cap/escalade

零堆分配的 CP 原生父目录向上遍历器，Clean-Room MIT 实现对标 npm 顶流包 `escalade`（1.2 亿+/周下载，`yargs` / `find-up` 核心依赖），跨包零重复复用 `@cap/normalize-path`。

## 特性

- `AncestorWalker`：从任意 POSIX 或 Windows 路径逐级向上遍历至根目录（`/` 或 `C:/`）
- 跨包复用 `@cap/normalize-path` 统一反斜杠与尾斜杠规范化
- `joinCandidate(dir, name, out)`：零堆拼接候选文件路径（如查找 `cap.json` / `package.json`）

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`lukeed/escalade`](https://github.com/lukeed/escalade)。
