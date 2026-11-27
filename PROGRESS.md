# CP 生态开发交接文档（PROGRESS.md）

> 更新时间：2026-11-27 15:45 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：**Top-2000 生态重组（Phase 1~5）+ 全仓 3,530 文件 `cpc check` 100% 绿灯 + 全生态 904 包 L0 标准库去重（Waves 1~16）100% 清零 + 38 大内置标准库 `-m` 深测绿灯 全面达成！** 全仓共 **904 个零重复生态包**（824 裸原名 + 8 `@org/pkg` + 39 原生 C/C++ `@cap/*` + 33 `@web/*`），`Uses cap:* stdlib` 达 **284**（从 0 跃升），**904/904** 完备 `src/index.d.cp` + `README.md` + `README.zh-CN.md` + `README_AGENT.md`。

---

## 1. 当前核心成果总览（Top-2000 生态重组 + 标准库全绿 + 904 包 L0 去重 100% 清零）

| 阶段 | 状态 | 核心交付与实测指标 |
| :--- | :---: | :--- |
| **Phase 1：L0 硬件 SIMD 基座 + 审计门禁** | ✅ 完成 | 新建 `cap:ascii` / `cap:utf8` / `cap:fmt` / `cap:bin`（208 内置通道）+ `scripts/audit_ecosystem.py`。3 大热路径升级为 16-lane `@Vector(16, u8)` SIMD；`tinybench` ReleaseFast 实测 4KB：`bytesEq` **14.07x** (161ns)、`isAscii` **14.64x** (126ns)、`findByte` **7.65x** (174ns)、`isValidUtf8` **146ns**。 |
| **Phase 2：裸包名零迁移改造 + 双平台合并** | ✅ 完成 | 832 包迁至原名 `packages/<orig>/`，合并 `bytes`/`semver` 及 6 处跨包符号，清理 27 组重复；`cpc/src/resolver.rs` 支持裸名 + `_`/`-` 双向归一化；全仓 **0 broken imports**。 |
| **Phase 3：全生态 L0 去重 Waves 1~16 (100% 清零)** | ✅ 完成 | 累计重构 **230+ 个裸包**，净消除 **8,500+ LOC** 重复手写辅助代码，`Uses cap:* stdlib` **0 → 284**；Wave 13 (`n-z` 清零) + Wave 14~16 (`a-m` 清零) 全部 **100% PASS** 并推送 `origin/main`（`4603fc51d`）。 |
| **Phase 4：SIMD / LUT / `undefined` 性能与所有权加固** | 🚀 深化中 | 生态核心包 + 内置 `lib/` Top-10（26 文件）已升级 16-lane SIMD、Slice-by-4 LUT 与 `undefined`（`undefined=375`）；现推进 Phase 4B（Worker AE/AF 双路清理全仓最后 43 个 `[0; N>=32]` 大栈清零包）。 |
| **Phase 5：全仓 3,530 文件 100% `cpc check` + 38 模块 `-m` 绿灯** | ✅ 完成 | **904/904 生态包** + **2,626/2,626 纯 CP 标准库** `cpc check` 100% PASS；全仓 `lib/*/index.cp` 导出冲突 **6 → 0**、`?? unreachable` **13 → 0**、批量升级 **197 个标准库文件 `static fn init`**，38 大核心内置标准库 `-m` 深测全绿。 |
| **下游 `bun_agy` 紧急工单热修** | ✅ 完成 | 结单 `task-bun-20261127-enum-slice-by-val` 与 `task-bun-20261127-zero-arg-call`，已发布 release `cpc` 并同步 `cpc_dev`；当前工单队列 **0 积压**。 |

---

## 2. 当前进行中与待办项

| 序号 | 项目 | 当前状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | **Phase 4B 性能深化 Batch 5 (Top 10 大栈清零 + SIMD)** | 🚀 进行中 (Worker AE) | `split2` (8KB) / `pkcs1` / `rand_hc` / `camino` / `picocolors` / `d3-path` / `publicsuffix` / `snakecase-keys` / `decamelize-keys` / `iri-string`。 |
| 2 | **Phase 4B 性能深化 Batch 6 (12 包大栈清零 + SIMD)** | 🚀 进行中 (Worker AF) | `pkg-dir` / `find-up` / `read-pkg-up` / `postcss` / `unist-util-visit` / `damerau-levenshtein` / `indexmap` / `simplify-js` / `id-arena` / `csv-core` / `relative-path` / `fast-fifo`。 |
| 3 | **全量生态包文档刷新 (`gen_pkg_docs.py`)** | 📋 待执行 | Batch 5~7 完工后一键刷新全部 904 包 `README_AGENT.md` / `README.zh-CN.md`。 |
| 4 | **`capdux.org` LIVE 批量同步** | ⏸️ 等待 `CAPDUX_TOKEN` | D1 免费额度已重置，当前 shell 环境未注入 `CAPDUX_TOKEN`，待提供 Token 后可一键全量同步 904 包至线上。 |
| 5 | **工作区既有机密/脏文件（切勿 `git add`）** | ⚠️ 注意 | `TASK.md`, `D PROGRESS_H.md`, `main_inc.zcs`, `repro_pack_modular_build`, `scripts/__pycache__/` 始终使用精确路径 `git add`。 |

---

## 3. 关键架构与避坑铁律（已固化至 `GEMINI.md`）

1. **串行 `cap test`**：多路同时运行 `cap test` 会争抢 `.cap/zig-cache` 文件锁导致 240x 降速（650ms → 160s），始终串行执行并加 `set -eo pipefail`（防 `| tail` 掩盖非零退出码）。
2. **L0 跨通道命名唯一**：不同 `cap:*` 通道禁止导出同名顶层函数（如 `cap:ascii.findByte` vs `cap:bin.findByteInBytes`）；`cap:fmt.writeStr(s, out, pos)` 参数顺序 `s` 在前；`cap:bin.readU32LE` 返回 `?u32`。
3. **测试文件 `fn strEq` 重名**：当包开始复用 `cap:ascii.strEq` 时，测试文件内手写 `fn strEq` 会展平重名，改为 `import { strEq } from "cap:ascii";` 即可。
4. **`.gitignore` 忽略 `bin/`**：新增 `lib/bin/*` 时需使用 `git add -f lib/bin/index.cp`。
5. **多 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`（`agy-comm.service` 常驻 `active`），定时任务每 3 小时审计 Token 效率并固化至 `GEMINI.md`。
