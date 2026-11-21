# `cap:env_path` — 跨平台 `PATH` 环境变量构建器与零分配迭代器

源自 Bun `paths/EnvPath.cp`，提供带 `~init()` 确定性 RAII 释放的 `EnvPath` 构建器（自动去重分隔符与尾部斜杠、支持 `append`/`appendUnique`/`prepend`/`contains`/`clear` 与自定义分隔符）以及零分配 `EnvPathIterator`。

## 导出符号

- `DEFAULT_DELIMITER`: 平台默认路径分隔符（POSIX `:` / Windows `;`）
- `EnvPathIterator`: `init` / `next`
- `EnvPath`（含 `~init()` RAII）: `init` / `initWithDelimiter` / `slice` / `len` / `isEmpty` / `clear` / `iterator` / `contains` / `append` / `appendUnique` / `prepend`
