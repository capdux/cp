# CP 生态开发交接文档（PROGRESS.md）

> 生成时间：2026-11-27 08:10 (+11:00) | 历史交接归档：`PROGRESS_H1.md`、`PROGRESS_H2.md`
> 当前仓库状态：`packages/@cap/` 共 **700 个官方包**（`@cap=700`，`@web=33`，总计 **733 个官方包**），`packages/@cap/registry.json` 共 **911 条上游映射**（`npm=451`，`crates.io=460`，接近精确 1:1 均衡）；全仓 `cap.json` `"source"` 溯源覆盖率 **100.0%（0 缺失）**。
> `capdux.org` Worker（`/data/dev/cap`，commit `1c734ce`）已部署 LIVE（支持按原库名/别名检索并展示 `from npm:<orig>` / `crates.io:<orig>` 与改名提示）。

---

## 1. 当前阻塞与未完成项

| 序号 | 项目 | 当前状态 | 说明 / 解除条件 |
| :--- | :--- | :--- | :--- |
| 1 | **`capdux.org` LIVE 批量发布（Batch 116 ~ 132）** | ⏸️ 等待 Cloudflare D1 每日免费行写入额度重置 | 当前线上 LIVE 为 **655 包**（Batch 115）；Batch 116~132 新增约 78 包待同步。Cloudflare D1 免费版每日 10 万行写入额度于 **UTC 00:00（北京时间 08:00 / 悉尼 11:00）重置**，重置后一次性运行 `seed_official.ts --live` 即可全部上线。 |
| 2 | **剩余 3 组含反向依赖的后缀重复包评估收口** | 📋 待评估 | 0 反向依赖的 14 组后缀重复包（Batch 130 清理 12 组 + Batch 131 清理 `ms-parse`/`pem-parse`）已全部收口。剩余 3 组存在 1~2 处调用方：`etag` vs `etag-http`（被 `serve-static`/`send` 引用）、`memchr` vs `memchr-crate`（被 `bytecount` 引用）、`p-limit` vs `p-limit-core`（被 `p-locate` 引用），新会话可连同调用方一并迁移并回归单测。 |
| 3 | **npm Top 700 & crates.io Top 700 持续吸收** | 🚀 进行中（912 / 1400 映射） | 继续保持 npm : crates.io ≈ 1:1 均衡，优先吸收零重复高下载独立包（已验空候选见第 4 节）。 |
| 4 | **工作区遗留未提交改动（非本任务）** | ⚠️ 切勿误提交 | `TASK.md` 与 `cpc/src/migrate/ts.rs` 为既有脏文件，提交时始终使用精确路径 `git add`。 |

---

## 2. 已排除的死胡同（避坑记录）

1. **Cloudflare D1 免费额度高频单包/单波全量 `seed_official.ts --live`**：每次全量 seed 会重写数百包 + 数百映射行，高频执行会触发 `D1_ERROR: exceeded D1's free tier daily row write limit`。**避坑**：本地先攒 5~10 个 Batch 再合并执行一次 `--live` 同步，或等待 UTC 00:00 额度重置后单次批量推送。
2. **盲目创建 `-crate`/`-core`/`-http`/`-parse` 后缀包导致生态重复**：早期未查 `packages/@cap/` 直接派生后缀包，产生过 15+ 组重复（如 `murmur3-core`、`ms-parse`、`pem-parse`、`aho-corasick-dfa` 等）。**避坑**：已固化进 `GEMINI.md` Rule #12，编码前必须先查 `lib/` + `packages/@cap/` + `registry.json`。
3. **单子智能体连续跑 20+ 包或 7 小时不轮换**：上下文无限累积导致每轮重传海量 Token 甚至卡死。**避坑**：已固化进 `GEMINI.md` Rule #9，每完成 **6~8 个包**立即 `kill` 并派生新子智能体。
4. **CP 1-Pass Codegen 高频踩坑语法**（均已固化进 `GEMINI.md` Rule #11）：
   - `let mut`（废除，直接用 `let`）、`let _ =`（用 `* = expr;`）、`if (opt) |v|`（用 `if (v := opt)`）、内联 `let x = if (...) { a } else { b }`（改用 `let x = a; if (...) { x = b; }`）；
   - 结构体方法误用 `.startsWith`/`.endsWith` 会被 `cpc` 拦截成 `std.mem.startsWith`（改用 `hasPrefix`/`hasSuffix`）；
   - 链式 `Type.staticFn(...).method(...)` 易导致类型推断退化为 `*anyopaque`（先绑定局部变量再调用）；
   - 无 receiver 工厂方法忘加 `static` 会报 `E0219`，只读方法写 `(_)` 会报 `E0123`；
   - 测试文件忘记包在 `fn main() { test(...); }` 内会导致 Zig 报 `root has no member named 'main'`。
5. **跨包模块展平符号冲突**：多个 `@cap/*` 被同一测试导入时，同名私有辅助函数或与导入顶层函数重名的结构体方法会触发 Zig 歧义报错。**避坑**：所有私有函数统一加 3~4 字符包前缀（如 `cam`、`mit`、`psl`、`mac`、`nid`）。

---

## 3. 关键约定与约束（已全部固化写入 `GEMINI.md`）

- **Rule #9（子智能体防膨胀轮换）**：保持 2 路活跃子智能体（NPM + Crates.io）+ 1 路主智能体并行；每完成 **6~8 个包**强制 `kill` 换新。
- **Rule #10（每波闭环交付）**：更新 `packages/@cap/registry.json`、`cp_lib_ref.md`、`CHANGELOG.md`、`DEVLOG.md` → 精确 `git add` + `git commit` + `git push origin main`（严禁日常读写万行 `cp_ref.md`）。
- **Rule #11（首轮一遍过 CP 编码清单 & 省 Token 铁律）**：本次交接已将最新 1-pass codegen 避坑规则与批量合并工具调用铁律全部补齐写入 `GEMINI.md`。
- **Rule #12（生态溯源、检索对齐与防重复铁律）**：
  1. 阶段汇报一律使用结构化 Markdown 表格；
  2. 每个 `cap.json` 必须包含 `"source": { "registry", "name", "repo", "license" }`；
  3. 每个 `README.md` 顶部必须包含 `<!-- cap-source-attribution -->` 溯源横幅；
  4. `capdux.org` 搜索 API 与前端均支持按原库名/别名检索并展示原库来源与改名提示；
  5. 吸收目标包或分离独立依赖包前，必须先查 `lib/` + `packages/@cap/` + `registry.json`（高频通用进 `lib/`，已有则复用/合并，严禁新建重复后缀包）。
- **跨 AGY 通信与 3 小时审计**：遵循 `/root/.agy/comm/COMM.md`，使用 `python3 /root/.agy/comm/task_queue.py unacked --agent cp_agy --json` 响应下游编译器工单；每 3 小时定时任务审计 Token 与效率。

---

## 4. 下一步行动（Next Step）

新会话启动后，按以下顺序立即执行：

1. **重建并行编队与 3 小时守护定时任务**：
   - 检查下游工单：`python3 /root/.agy/comm/task_queue.py unacked --agent cp_agy --json`。
   - 派生 2 路全新子智能体（`Worker N15 (NPM Top-700)` + `Worker C14 (Crates Top-700)`），并重建每 3 小时 `schedule` 守护审计任务（`0 */3 * * *`）。
   - **已验空零重复首批推荐包**（编码前仍需再确认 `lib/`）：
     - NPM：`@cap/dequal`（深度相等）、`@cap/klona`（零堆定容深拷贝语义）、`@cap/is-unicode-supported`、`@cap/is-ci`；
     - Crates.io：`@cap/normpath`（跨平台路径规范化）、`@cap/humantime-serde`（复用 `@cap/humantime`）。
2. **UTC 00:00 后执行单次 `capdux.org` LIVE 同步**：
   - 目标脚本：`/data/dev/cap/scripts/seed_official.ts`（`--live --registry https://registry.capdux.org`），一次性将 Batch 116~132（655 → **733 官方包** + **911 条上游映射**）全量推送上线。
3. **收口剩余 3 组后缀重叠包**（`etag-http`→`etag`、`memchr-crate`→`memchr`、`p-limit-core`→`p-limit`），更新其 1~2 个调用方 `import` 与 `cap.json` 并跑通回归单测。

---

## 5. 经验总结（高吞吐、低 Token 实战方法论）

1. **三路并行流水线 + 6 包轮换**：2 路子智能体专注写包与单测（不碰 `registry.json` 和 `git` 避免冲突），主智能体负责自身写包 + 汇总 `registry.json`/文档 + 统一 `git commit & push` + 审核与轮换，吞吐可达每小时 10~15 个高质量 6/6 PASS 独立包且上下文永不爆炸。
2. **批处理脚本替代逐个编辑**：批量回填 688 个包的 `cap.json` `"source"` 与 `README.md` 溯源横幅，用单次 Python 脚本 2 秒完成并自动校验 `still_missing=0`，节省数十万 Token。
3. **复用优先于重写**：如 `@cap/kebabcase-keys` 直接复用 `@cap/decamelize-keys` + `@cap/map-obj`、`@cap/cli-truncate` 复用 `@cap/string-width` + `@cap/slice-ansi`、`@cap/camino` 复用 `@cap/utf8-valid`，代码量减半、单测一遍过率接近 100%。
4. **善用 `cpc` AI-First 诊断**：遇到编译错误先跑一次 `cpc check <file>` 一次性看全报错，集中修复后再 `cpc emit-types && cap test`，避免盲猜往返。
5. **审计脚本兜底留档**：每次里程碑与交接前运行 `python3 scripts/sync_session_audit.py --check`，确保 `SESSION.md` / `SESSION_H.md` 中用户原始指令、下游 AGY 工单与完整回复 100% 零遗漏。
