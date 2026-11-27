# CP 生态开发交接文档（PROGRESS.md）

> 更新时间：2026-11-27 13:40 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：**Top-2000 生态重组（Phase 1~5）主体全面收官！** 全仓共 **904 个零重复生态包**（824 裸原名 + 8 `@org/pkg` + 39 原生 C/C++ `@cap/*` + 33 `@web/*`），`packages/registry.json` 覆盖 npm + crates.io Top 1000 映射（不保留旧 `@cap/<old>` 别名），全仓 **0 broken imports**，**904/904** 完备 `src/index.d.cp` + `README.md`（含中英文切换栏）+ `README.zh-CN.md` + `README_AGENT.md`。

---

## 1. 当前核心成果总览（Top-2000 生态重组 5 阶段）

| 阶段 | 状态 | 核心交付与实测指标 |
| :--- | :---: | :--- |
| **Phase 1：L0 硬件 SIMD 基座 + 审计门禁** | ✅ 完成 | 新建 `cap:ascii` / `cap:utf8` / `cap:fmt` / `cap:bin`（208 内置通道）+ `scripts/audit_ecosystem.py`。3 大热路径升级为 16-lane `@Vector(16, u8)` SIMD；`tinybench` ReleaseFast 实测 4KB：`bytesEq` **14.07x** (161ns)、`isAscii` **14.64x** (126ns)、`findByte` **7.65x** (174ns)、`isValidUtf8` **146ns**。 |
| **Phase 2：裸包名零迁移改造 + 双平台合并** | ✅ 完成 | 832 包迁至原名 `packages/<orig>/`，合并 `bytes`/`semver` 及 6 处跨包符号，清理 27 组重复；`cpc/src/resolver.rs` 支持裸名 + `_`/`-` 双向归一化；全仓 **0 broken imports**。 |
| **Phase 3：L0 去重 5 Waves 全清零** | ✅ 完成 | Wave 1~5 重构 75+ 高频包，净消除 **3,500+ LOC** 重复手写辅助代码，`Uses cap:* stdlib` **0 → 128+**，全生态 `>=2` 重复 helper 包 **100% 清零**。 |
| **Phase 4：SIMD / LUT / `undefined` 性能与所有权加固** | ✅ 完成 | 4 批核心包（Batch 1~4）+ 主智能体：`memchr`/`simdutf8`/`adler2`/`simd-adler32`/`bytecount` 升级 16-lane `@Vector(16, u8)` SIMD；`crc32fast`/`crc16` 升级 256-entry LUT；`base64ct` 消除 4 处尾比特分支实现真无分支恒定时间；累计 `undefined=369+` 零清零栈缓冲；P0 核心包 100% 所有权/借用安全终审。 |
| **Phase 5：全量文档 + AI-First CLI + 904 包体检** | 🚀 收尾 | 904/904 文档与 `.d.cp` 100% 覆盖；`cap` CLI 交付 `cap brief <pkg|cap:stdlib>` + `cap fix [--dry-run] <path>` + `cap check <path>`；L0 + Top 20 回归 **20/20 PASS**；全仓 904 包 `cpc check` 体检 **0 语法/借用/导入错误**（858/904 通过，仅 46 例为 `cpc/src/sema/` 类型推断假阳性，正在 `cpc_dev/` 集中修复）。 |

---

## 2. 当前进行中与待办项

| 序号 | 项目 | 当前状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | **`cpc_dev/src/sema/` 修复 6 类静态类型假阳性（46 → 0）** | 🚀 进行中 (Worker L) | 修复完成后合并至 `cpc/src/`，执行 `bash scripts/sync_cpc.sh release` 发布并广播通知下游 `bun_agy`/`capsurf_agy`，实现全仓 **904/904 (100%)** `cpc check` 绿灯。 |
| 2 | **单-helper 深度去重（10 包）** | 🚀 进行中 (Worker K) | 清理残留单处 `hexVal`/`strEq` 热点包，继续提升 `cap:ascii`/`cap:bin`/`cap:fmt` 复用率。 |
| 3 | **Phase 5 最终文档刷新** | 📋 紧接执行 | Worker K 完工后运行 `python3 scripts/gen_pkg_docs.py --apply --force` 刷新全部 904 个包的 `README_AGENT.md` / `README.zh-CN.md` / `src/index.d.cp`。 |
| 4 | **`capdux.org` LIVE 批量同步** | ⏸️ 等待 `CAPDUX_TOKEN` | D1 免费额度已重置，当前 shell 环境未注入 `CAPDUX_TOKEN`，待提供 Token 后可一键全量同步 904 包至线上。 |
| 5 | **工作区既有机密/脏文件（切勿 `git add`）** | ⚠️ 注意 | `TASK.md`, `D PROGRESS_H.md`, `main_inc.zcs`, `repro_pack_modular_build`, `scripts/__pycache__/` 始终使用精确路径 `git add`。 |

---

## 3. 关键架构与避坑铁律（已固化至 `GEMINI.md`）

1. **串行 `cap test`**：多路同时运行 `cap test` 会争抢 `.cap/zig-cache` 文件锁导致 240x 降速（650ms → 160s），始终串行执行并加 `set -eo pipefail`（防 `| tail` 掩盖非零退出码）。
2. **L0 跨通道命名唯一**：不同 `cap:*` 通道禁止导出同名顶层函数（如 `cap:ascii.findByte` vs `cap:bin.findByteInBytes`）；`cap:fmt.writeStr(s, out, pos)` 参数顺序 `s` 在前；`cap:bin.readU32LE` 返回 `?u32`。
3. **测试文件 `fn strEq` 重名**：当包开始复用 `cap:ascii.strEq` 时，测试文件内手写 `fn strEq` 会展平重名，改为 `import { strEq } from "cap:ascii";` 即可。
4. **`.gitignore` 忽略 `bin/`**：新增 `lib/bin/*` 时需使用 `git add -f lib/bin/index.cp`。
5. **多 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`（`agy-comm.service` 常驻 `active`），定时任务每 3 小时审计 Token 效率并固化至 `GEMINI.md`。
