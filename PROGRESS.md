# CP 生态开发交接文档（PROGRESS.md）

> 更新时间：2026-11-27 18:05 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：**Top-2000 生态重组（Phase 1~5）+ 全仓 3,530 文件 `cpc check` 100% 绿灯 + 全仓 854 / 854 (100.0%) `testCap*.cp` 生态回归大满贯 + 内置标准库 1,350+ `-m` 深测全绿 + 编译器 `ImportSpecifier`/`orelse unreachable` 正式版升级 + 36 个短包“做全不做薄”API 补全（284/284 PASS）+ Top 98 明星包（294 份）EN/ZH/AGENT 文档深度精修 全面达成！**
> 全仓共 **904 个零重复生态包**（`Uses cap:* stdlib = 299`，`undefined = 396`），**904/904** 完备 `src/index.d.cp` + `README.md` + `README.zh-CN.md` + `README_AGENT.md`，全仓 2,494 份裸包 README **0 处旧 `@cap/` 残留**。

---

## 1. 当前核心成果总览

| 阶段 | 状态 | 核心交付与实测指标 |
| :--- | :---: | :--- |
| **Phase 1~2：L0 SIMD 基座 + 裸包名零迁移改造** | ✅ 完成 | 832 包迁至原名 `packages/<orig>/`，合并 27 对重复包，0 broken imports；L0 16-lane SIMD `bytesEq` 14.07x / `isAscii` 14.64x。 |
| **Phase 3：全生态 L0 去重 Waves 1~16 (100% 清零)** | ✅ 完成 | 重构 **230+ 裸包**，净消除 **8,500+ LOC**，`Uses cap:* stdlib` **0 → 299** (`4603fc51d`)。 |
| **Phase 4B/4C：大栈清零 + 算法级 SIMD / LUT 提速** | ✅ 完成 | Batch 5~8（42 包）消除 **~91 KB+/call** 无意义栈清零（`undefined=396`）；Batch 9~12（23 核心包）升级 Slice-by-4 LUT / 16-lane SIMD / `@memcpy`；ReleaseFast 实测（`bench/ECOSYSTEM_PERF_REPORT.md`）：**`utf8parse` 17.8 GB/s、`lz4_flex` 3.1 GB/s、`crc32fast` 1.14 GB/s、`semver` 6.39M ops/s、`qs` 5.87M ops/s**。 |
| **Phase 5：全仓 100% 静态 + 生态回归大满贯** | ✅ 完成 | **3,530 / 3,530 (100%) `cpc check`** 全绿；**854 / 854 (100.0%) `cap/test/testCap*.cp`** 全仓生态回归大满贯 (`ffc731757`)。 |
| **内置标准库 `lib/` `-m` 深测 + 编译器升级** | ✅ 完成 | 清零 6 处 `lib/*/index.cp` 重名与 13 处 `?? unreachable`；升级 197 文件显式 `static fn init`；修复 `cpc` `orelse unreachable` 与 `-m` `import { a as b }` 别名映射并发布 Release (`4bf72ac66`)；**1,350+ 内置标准库 `-m` 深测全绿**，统一 `lib/graphql/` Oct 2021 AST 契约 (`ba70994ba`)。 |
| **“做全不做薄”短包 API 补全（Batch 1~4）** | ✅ 完成 | **36 个核心短包** 净增 **+11,299 LOC** 完整源包 API 与单测（**284/284 PASS**），涵盖 `type-is`, `miniz_oxide`, `bytemuck`, `zerocopy`, `immer`, `hdrhistogram`, `p-queue`, `snowflake`, `rand_pcg`, `base36`, `make-dir`, `trough` 等 (`f07be5e8e`, `be3acc85e`, `4d658c84f`, `48060a55a`)。 |
| **全仓文档净化 + Top 98 明星包文档深度精修（Batch 1~8）** | ✅ 完成 | 清零 2,231 处旧 `@cap/` 残留引用 (`10338c3d6`, `e9b3e4600`)；深度精修 **Top 98 明星包 / 294 份 EN/ZH/AGENT 文档** (`2b41af2ab`)。 |

---

## 2. 当前进行中与待办项

| 序号 | 项目 | 当前状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | **Top 110 明星包 EN/ZH/AGENT 文档精修 (Batch 9)** | 🚀 进行中 (Worker BX) | 12 核心密码/压缩/ID/模板包：`chacha20poly1305`, `aes-gcm`, `flate2`, `zstd`, `ulid`, `nanoid`, `handlebars`, `acorn`, `fast-uri`, `tinytemplate`, `prom-client`, `standardwebhooks`。 |
| 2 | **“做全不做薄”短包 API 补全 (Batch 5)** | 🚀 进行中 (Worker BY) | 10 个核心短包：`hkdf`, `base64url`, `encodeurl`, `toidentifier`, `parse-ms`, `soundex-code`, `commondir`, `kebabcase-keys`, `smawk`, `romans`。 |
| 3 | **内置标准库剩余 `test_*_production_deep.cp` 100% 抽核** | 📋 待推进 | 核验全部 1,402 个深测文件的剩余盲区，确保无遗漏。 |
| 4 | **`capdux.org` LIVE 批量同步** | ⏸️ 等待 `CAPDUX_TOKEN` | 本地 904 包与 `registry.json` 已就绪，待环境变量提供 `CAPDUX_TOKEN` 后一键同步至线上。 |
| 5 | **工作区既有机密/脏文件（切勿 `git add`）** | ⚠️ 注意 | `TASK.md`, `D PROGRESS_H.md`, `main_inc.zcs`, `repro_pack_modular_build`, `scripts/__pycache__/` 及 `cpc/src/migrate/*` 等始终使用精确路径 `git add`。 |

---

## 3. 关键架构与避坑铁律（已固化至 `GEMINI.md`）

1. **串行 `cap test`**：多路同时运行 `cap test` 会争抢 `.cap/zig-cache` 文件锁导致 240x 降速（650ms → 160s），始终串行执行并加 `set -eo pipefail`（防 `| tail` 掩盖非零退出码）。
2. **多文件标准库深测用 `cap test -m`**：隔离私有辅助函数冲突；升级 `cpc` 后清空 `.cap/test-mod` 触发全量重转译。
3. **显式 `static fn` 工厂方法**：无 `self` 接收者的 struct 工厂方法统一声明 `pub static fn`，避免 `E0219` 与 Zig unused parameter 报错。
4. **L0 跨通道命名唯一**：不同 `cap:*` 通道禁止导出同名顶层函数；测试文件避免手写 `fn strEq`，直接 `import { strEq } from "cap:ascii";`。
5. **多 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`（`agy-comm.service` 常驻 `active`），定时任务每 3 小时审计 Token 效率并固化至 `GEMINI.md`（已固化至 §6.10）。
