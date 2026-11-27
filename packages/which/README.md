# @cap/which-bin

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:which` ([https://github.com/npm/node-which](https://github.com/npm/node-which))（原库名 `which` → CP 包名 `@cap/which-bin`）

零堆分配的 CP 原生跨平台 `PATH` 可执行文件候选路径解析器，Clean-Room MIT 实现对标 npm Top 20 包 `which`（2 亿+/周下载），跨包复用 `@cap/normalize-path`。

## 特性

- `WhichIterator.newPosix(cmd, path_env)`：按 `:` 切分并归一化每个 `PATH` 目录生成候选路径
- `WhichIterator.newWindows(cmd, path_env, pathext)`：支持 `;` 分隔、引号剥离与 `PATHEXT`（`.EXE;.CMD;.BAT;.COM`）自动展开
- `isExplicitPath`、`hasPathExt`、`countPathDirs`、`pathDirAt` 零堆工具函数

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`npm/node-which`](https://github.com/npm/node-which)。
