# CP 生态开发交接文档（PROGRESS.md）

> 更新时间：2026-11-27 16:40 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：**Top-2000 生态重组（Phase 1~5）+ 全仓 3,530 文件 `cpc check` 100% 绿灯 + 全仓 854 / 854 (100.0%) `testCap*.cp` 生态回归大满贯 + Phase 4B/4C SIMD/LUT/`@memcpy` 极致提速 + 16 个短包“做全不做薄”API 补全 全面达成！** 全仓共 **904 个零重复生态包**（`Uses cap:* stdlib = 299`，`undefined = 396`），**904/904** 完备 `src/index.d.cp` + `README.md` + `README.zh-CN.md` + `README_AGENT.md`。

---

## 1. 当前核心成果总览

| 阶段 | 状态 | 核心交付与实测指标 |
| :--- | :---: | :--- |
| **Phase 1~2：L0 SIMD 基座 + 裸包名零迁移改造** | ✅ 完成 | 832 包迁至原名 `packages/<orig>/`，合并 27 对重复包，0 broken imports；L0 16-lane SIMD `bytesEq` 14.07x / `isAscii` 14.64x。 |
| **Phase 3：全生态 L0 去重 Waves 1~16 (100% 清零)** | ✅ 完成 | 重构 **230+ 裸包**，净消除 **8,500+ LOC**，`Uses cap:* stdlib` **0 → 299**。 |
| **Phase 4B/4C：大栈清零 + 算法级 SIMD / LUT 提速** | ✅ 完成 | Batch 5~8（42 包）消除 **~91 KB+/call** 无意义栈清零（`undefined=396`）；Batch 9~12（23 核心包）升级 Slice-by-4 LUT / 16-lane SIMD / `@memcpy`；ReleaseFast 实测（`bench/ECOSYSTEM_PERF_REPORT.md`）：**`utf8parse` 17.8 GB/s、`lz4_flex` 3.1 GB/s、`crc32fast` 1.1 GB/s、`semver` 6.39M ops/s、`qs` 5.87M ops/s**。 |
| **Phase 5：全仓 100% 静态 + 运行态大满贯** | ✅ 完成 | **3,530 / 3,530 (100%) `cpc check`** 全绿；**854 / 854 (100.0%) `cap/test/testCap*.cp`** 全仓生态回归大满贯（`ffc731757`）；38 大内置标准库 `-m` 深测全绿。 |
| **“做全不做薄”短包 API 补全（Batch 1~2）** | ✅ 完成 | 16 个关键短包（`type-is`, `dom-accessibility-api`, `zod-validation-error`, `miniz_oxide`, `p-map`, `bit_field`, `deranged`, `bytemuck`, `hdrhistogram`, `p-queue`, `zerocopy`, `immer`, `schema-utils`, `jest-docblock`, `finalhandler`, `media-typer`）净增 **+6,635 LOC** 完整源包 API 与单测（**124/124 PASS**）。 |

---

## 2. 当前进行中与待办项

| 序号 | 项目 | 当前状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | **Top 12 明星包 EN/ZH/AGENT 文档深度精修 (Batch 1)** | 🚀 进行中 (Worker AQ) | `semver`, `bytes`, `picomatch`, `url`, `qs`, `lz4_flex`, `crc32fast`, `crc32c`, `utf8parse`, `marked`, `diff`, `chalk`。 |
| 2 | **Top 12 明星包 EN/ZH/AGENT 文档深度精修 (Batch 2)** | 🚀 进行中 (Worker AR) | `ms`, `lru-cache`, `minimatch`, `tough-cookie`, `ini`, `snap`, `dotenv-expand`, `toml_edit`, `percent-encoding`, `postcss`, `ajv`, `regex-lite`。 |
| 3 | **`capdux.org` LIVE 批量同步** | ⏸️ 等待 `CAPDUX_TOKEN` | 本地 904 包与 `registry.json` 已就绪，待环境变量提供 `CAPDUX_TOKEN` 后一键同步至线上。 |
| 4 | **工作区既有机密/脏文件（切勿 `git add`）** | ⚠️ 注意 | `TASK.md`, `D PROGRESS_H.md`, `main_inc.zcs`, `repro_pack_modular_build`, `scripts/__pycache__/` 始终使用精确路径 `git add`。 |

---

## 3. 关键架构与避坑铁律（已固化至 `GEMINI.md`）

1. **串行 `cap test`**：多路同时运行 `cap test` 会争抢 `.cap/zig-cache` 文件锁导致 240x 降速（650ms → 160s），始终串行执行并加 `set -eo pipefail`（防 `| tail` 掩盖非零退出码）。
2. **L0 跨通道命名唯一**：不同 `cap:*` 通道禁止导出同名顶层函数（如 `cap:ascii.findByte` vs `cap:bin.findByteInBytes`）；`cap:fmt.writeStr(s, out, pos)` 参数顺序 `s` 在前；`cap:bin.readU32LE` 返回 `?u32`。
3. **测试文件 `fn strEq` 重名**：当包开始复用 `cap:ascii.strEq` 时，测试文件内手写 `fn strEq` 会展平重名，改为 `import { strEq } from "cap:ascii";` 即可。
4. **`.gitignore` 忽略 `bin/`**：新增 `lib/bin/*` 时需使用 `git add -f lib/bin/index.cp`。
5. **多 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`（`agy-comm.service` 常驻 `active`），定时任务每 3 小时审计 Token 效率并固化至 `GEMINI.md`。
