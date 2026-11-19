# PROGRESS.md - Project Handover

## 1. 当前阻塞与未完成项
- **Zig 0.17.0 转译兼容性**：CP 编译器生成的 Zig 代码在 Zig 0.17 下出现大量转译失败，表现为：
    - 关键字冲突（如 `pub const test`，Zig 0.17 禁止该名称）。
    - 语法语义不兼容（如 optional 类型 `?T` 被废除）。
    - 类型系统 mismatch（如类型名与内建类型冲突）。
    - 借用检查与 async/await 的边界冲突（async 代码跨界借用）。
- **DAG 导入解析引擎完成度 80%**：已初步重构 `main.zig` 对 `build-exe` 的判定，需进一步完善 `cap` 解析 Import DAG 的递归逻辑，确保所有 `cap:*` 依赖链能正确冒泡至根节点。
- **`/data/dev/cp/lib/cap.json` 索引集成**：需在 `cap` 工具中正式引入对该中央索引文件的读取逻辑，将其作为内置库的默认查找路径，替代硬编码的搜索逻辑。
- **全量交叉编译回归**：需在 Linux 环境下验证 `capsurf` 对 `aarch64` 等目标平台的交叉编译是否能根据 `bin_dependencies` 自动拉取/构建对应二进制。

## 2. 已排除的死胡同（避坑记录）
- **硬编码全局路径**：通过 `link_libs` 和 `-L` 强制链接宿主机 `/lib64/` 下的动态库方案已被否决（导致交叉编译环境污染和链接冲突）。
- **非自包含依赖**：拒绝了通过修改 `capsurf` 子项目 CMake/Makefile 以适应宿主机环境的做法（违反了构建确定性原则）。
- **Swarm Board 单点通信**：已弃用 Swarm 广播作为 AGY 间编译任务状态同步的唯一手段（响应延迟大且丢包），改用 `/.cp_sync/` 文件状态同步。
- **批量正则替换**：禁止通过 `sed` 或 `find` 批量修改 `vendor/` 下的源码（极易破坏 Bazel/CMake 构建一致性）。

## 3. 关键约定与约束
- **核心原则**：CP 构建必须具备**Hermetic（封闭性）**。依赖（Sys/Bin）必须在 `cap.json` 中显式声明，`cap` 构建器严禁隐式查找系统路径。
- **权限边界**：严禁修改 `capsurf` 及任何 `vendor/` 下的第三方源代码。所有修复必须在 `cpc` 或 `cap` 工具链中完成。
- **通信协议**：AGY 间协同（尤其是编译状态更新）必须通过 `/.cp_sync/status.json` 进行文件级同步。

## 4. 下一步行动（Next Step）
1.  **目标文件**：`/data/dev/cp/cap/src/main.zig` 及 `registry` 解析逻辑。
2.  **动作**：
    - 完成 Import DAG 递归扫描逻辑，实现按需添加 `bin_dependencies` 编译任务。
    - 对接 `cap` 与 `/data/dev/cp/lib/cap.json` 的索引解析。
3.  **最终验证**：对 `capsurf` 执行 `cap build -r -f`，确保零警告生成多平台二进制。

## 5. 高效运作建议
- **代码检索**：优先使用 `rg -n`，配合 `read_file` 精确读取上下文。
- **流程脚本化**：严禁在交互中拼接长命令，统一调用预置 Bash 脚本（如 `scripts/sync_cpc.sh`）。
- **文档冷热分离**：持续执行 `SESSION.md` 等四线文档的冷热归档，保持热文件轻量化，减少 Token 损耗。
- **状态同步**：后续 AGY 若需获取当前构建状态，直接读取 `/data/dev/capsurf/.cp_sync/status.json`，无需通过对话问询。
