# CP 生态开发交接文档（PROGRESS.md）

> 更新时间：2026-11-27 15:25 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：**Top-2000 生态重组（Phase 1~5）+ 全仓 3,530 文件 `cpc check` 100% 绿灯 + 内置标准库 Top-10 SIMD/LUT/`undefined` 提速 + 全仓 `lib/*/index.cp` 0 导出冲突 + 单-helper 生态去重 Wave 1~8 (80 包) 全面达成！** 全仓共 **904 个零重复生态包**（824 裸原名 + 8 `@org/pkg` + 39 原生 C/C++ `@cap/*` + 33 `@web/*`），`Uses cap:* stdlib` 达 **209**（从 0 跃升），**904/904** 完备 `src/index.d.cp` + `README.md` + `README.zh-CN.md` + `README_AGENT.md`。

---

## 1. 当前核心成果总览（Top-2000 生态重组 + 标准库全绿 + 持续深度去重）

| 阶段 | 状态 | 核心交付与实测指标 |
| :--- | :---: | :--- |
| **Phase 1：L0 硬件 SIMD 基座 + 审计门禁** | ✅ 完成 | 新建 `cap:ascii` / `cap:utf8` / `cap:fmt` / `cap:bin`（208 内置通道）+ `scripts/audit_ecosystem.py`。3 大热路径升级为 16-lane `@Vector(16, u8)` SIMD；`tinybench` ReleaseFast 实测 4KB：`bytesEq` **14.07x** (161ns)、`isAscii` **14.64x** (126ns)、`findByte` **7.65x** (174ns)、`isValidUtf8` **146ns**。 |
| **Phase 2：裸包名零迁移改造 + 双平台合并** | ✅ 完成 | 832 包迁至原名 `packages/<orig>/`，合并 `bytes`/`semver` 及 6 处跨包符号，清理 27 组重复；`cpc/src/resolver.rs` 支持裸名 + `_`/`-` 双向归一化；全仓 **0 broken imports**。 |
| **Phase 3：L0 去重 Waves 1~5 + 单-helper Waves 1~8** | ✅ 完成 | 累计重构 **155+ 个裸包**，净消除 **7,100+ LOC** 重复手写辅助代码，`Uses cap:* stdlib` **0 → 209**；Wave 7 (`a-m`, -595 LOC, 10/10 PASS) & Wave 8 (`n-z`, -456 LOC, 10/10 PASS) 已推送 `origin/main`（`d11e7d243`）。 |
| **Phase 4：SIMD / LUT / `undefined` 性能与所有权加固** | ✅ 完成 | 生态核心包 + 内置 `lib/` Top-10（26 文件：`url`/`http`/`csv`/`json`/`yaml`/`toml`/`codec/gf256`/`crypto/crc`/`pack`/`text`）全面升级 16-lane SIMD、Slice-by-4 LUT 与 `undefined` 零清零栈缓冲（`undefined=372`）。 |
| **Phase 5：全仓 3,530 文件 100% `cpc check` 绿灯 + 文档 + CLI** | ✅ 完成 | 修复 `cpc/src/sema/` 6 类假阳性（**904/904 生态包 100% PASS**）与 `lib/` 79 个历史文件（**2,626/2,626 纯 CP 标准库 100% PASS**）；修复全部 6 处 `lib/*/index.cp` 导出冲突与 13 处 `?? unreachable`；`cap brief/fix/check` 全部落地。 |
| **下游 `bun_agy` 紧急工单热修** | ✅ 完成 | 结单 `task-bun-20261127-enum-slice-by-val`（tagged enum 借用视图 UAR 修复）与 `task-bun-20261127-zero-arg-call`（零参大写方法保留 `()`），已发布 release `cpc` 并同步 `cpc_dev`。 |

---

## 2. 当前进行中与待办项

| 序号 | 项目 | 当前状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | **单-helper 生态去重 Wave 9 (`n-z` 10 包)** | 🚀 进行中 (Worker W) | `remove-markdown` / `standardwebhooks` / `trim-newlines` / `tinytemplate` / `prom-client` 等 10 包零冲突分片去重。 |
| 2 | **单-helper 生态去重 Wave 10 (`a-m` 10 包)** | 🚀 进行中 (Worker X) | `cronstrue` / `bl` / `convert_case` / `ident_case` / `is-number` / `fast-json-stable-stringify` / `expr-eval` 等 10 包零冲突分片去重。 |
| 3 | **内置标准库 `lib/` 深测 `-m` 逐模块清障** | 🚀 进行中 (主智能体) | 已修复 6 处 barrel 导出冲突 + 13 处 `?? unreachable` + `nanoid`/`nix`/`woff2`/`wgNoise`/`wgReplay` 静态方法与借用声明，持续清绿 `cap/test/test_*_production_deep.cp`。 |
| 4 | **`capdux.org` LIVE 批量同步** | ⏸️ 等待 `CAPDUX_TOKEN` | D1 免费额度已重置，当前 shell 环境未注入 `CAPDUX_TOKEN`，待提供 Token 后可一键全量同步 904 包至线上。 |
| 5 | **工作区既有机密/脏文件（切勿 `git add`）** | ⚠️ 注意 | `TASK.md`, `D PROGRESS_H.md`, `main_inc.zcs`, `repro_pack_modular_build`, `scripts/__pycache__/` 始终使用精确路径 `git add`。 |

---

## 3. 关键架构与避坑铁律（已固化至 `GEMINI.md`）

1. **串行 `cap test`**：多路同时运行 `cap test` 会争抢 `.cap/zig-cache` 文件锁导致 240x 降速（650ms → 160s），始终串行执行并加 `set -eo pipefail`（防 `| tail` 掩盖非零退出码）。
2. **L0 跨通道命名唯一**：不同 `cap:*` 通道禁止导出同名顶层函数（如 `cap:ascii.findByte` vs `cap:bin.findByteInBytes`）；`cap:fmt.writeStr(s, out, pos)` 参数顺序 `s` 在前；`cap:bin.readU32LE` 返回 `?u32`。
3. **测试文件 `fn strEq` 重名**：当包开始复用 `cap:ascii.strEq` 时，测试文件内手写 `fn strEq` 会展平重名，改为 `import { strEq } from "cap:ascii";` 即可。
4. **`.gitignore` 忽略 `bin/`**：新增 `lib/bin/*` 时需使用 `git add -f lib/bin/index.cp`。
5. **多 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`（`agy-comm.service` 常驻 `active`），定时任务每 3 小时审计 Token 效率并固化至 `GEMINI.md`。
