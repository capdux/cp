# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:rule` 声明式生产规则与决策引擎（5/5 PASS，文档直写 `doc/lib/rule.md`）

- **落地成果**：`lib/rule/`（`rule.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sheet`（电子表格 `A1` 坐标解析、DAG 公式拓扑自动重算、`#CYCLE!` 环路拦截与 CSV/JSON 导出引擎），与 `lib/ssh/` 保持 2 路常规并行。

---

## 2026-11-21: 紧急热修 `cpc` 吞掉 `export fn` / `export const` 问题并结单交付 `bun_agy`

- **落地成果**：排查发现 `lexer.rs` 将 `export`/`pub` 均映射为 `Token::Export` 且顶层 `VarDecl` 缺少 `is_export` 导致跨 C/C++ ABI 导出丢失。修复后已发布 `cpc` Release 版并通过 `zig ast-check` 与 `test_basic.cp` 验证，完成 `resolve` + `ack` 结单交付并回收热修子智能体，恢复 2 路常规并行。

---

## 2026-11-21: 新建 `cap:http_cache` RFC 9111 HTTP 缓存与 `ETag` 协商引擎（5/5 PASS，文档直写 `doc/lib/http_cache.md`）

- **落地成果**：`lib/http_cache/`（`cache_policy.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ssh`（OpenSSH 公钥 Wire 解析、`SHA256:` 指纹计算与 `authorized_keys` 策略匹配引擎），与 `lib/rule/` 保持 2 路常规并行。

---

## 2026-11-21: 新建 `cap:dedup` SimHash / MinHash / LSH 近重复去重引擎（5/5 PASS，文档直写 `doc/lib/dedup.md`）

- **落地成果**：`lib/dedup/`（`dedup.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:rule`（声明式业务风控与决策规则引擎：多条件树、Salience 优先级、互斥组与审计报告），与 `lib/http_cache/` 保持 2 路常规并行。

---

## 2026-11-21: 新建 `cap:highlight` 多语言词法高亮与 ANSI/HTML 渲染引擎（5/5 PASS，文档直写 `doc/lib/highlight.md`）

- **落地成果**：`lib/highlight/`（`highlight.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），与 `lib/vcard/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:http_cache`（RFC 9111 HTTP `Cache-Control` 解析、强弱 `ETag` 304 条件协商与新鲜度寿命引擎），与 `lib/dedup/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:vcard` RFC 6350 vCard 通讯录解析与生成引擎（5/5 PASS，文档直写 `doc/lib/vcard.md`）

- **落地成果**：`lib/vcard/`（`vcard.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:dedup`（SimHash 海明分片索引 + MinHash + LSH 近重复文本与 RAG 语料去重引擎），与 `lib/highlight/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sat` DPLL 布尔 SAT 求解器与约束建模引擎（5/5 PASS，文档直写 `doc/lib/sat.md`）

- **落地成果**：`lib/sat/`（`solver.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:highlight`（多语言代码词法高亮引擎：CP/Zig/Rust/TS/Python/JSON/SQL/Bash 词法切分与 ANSI/HTML 渲染），与 `lib/vcard/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:tween` 缓动插值、关键帧与弹簧物理动画引擎（5/5 PASS，文档直写 `doc/lib/tween.md`）

- **落地成果**：`lib/tween/`（`tween.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:vcard`（RFC 6350/2426 vCard 3.0/4.0 电子名片通讯录 `.vcf` 解析与生成引擎），与 `lib/sat/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:noise` 确定性 Perlin/Worley/fBm 噪声引擎（5/5 PASS，文档直写 `doc/lib/noise.md`）

- **落地成果**：`lib/noise/`（`noise.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sat`（DPLL + 单位传播布尔可满足性 SAT 求解器与依赖/排班约束建模器），与 `lib/tween/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:spell` SymSpell / BK-Tree / Soundex 拼写纠错引擎（5/5 PASS，文档直写 `doc/lib/spell.md`）

- **落地成果**：`lib/spell/`（`spell.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:tween`（24+ Easing 缓动曲线、关键帧轨道插值与二阶弹簧阻尼物理动画引擎），与 `lib/noise/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ecs` Sparse-Set 实体组件系统 ECS 引擎（5/5 PASS，文档直写 `doc/lib/ecs.md`）

- **落地成果**：`lib/ecs/`（`ecs.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:noise`（确定性 2D/3D Perlin 梯度噪声、Voronoi Cellular 噪声与 fBm 分形地形生成引擎），与 `lib/spell/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:barcode` Code 128 / EAN-13 / UPC-A 条码编码引擎（5/5 PASS，文档直写 `doc/lib/barcode.md`）

- **落地成果**：`lib/barcode/`（`barcode.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:spell`（SymSpell 对称删除索引、BK-Tree 编辑距离树与 Soundex 语音编码拼写纠错引擎），与 `lib/ecs/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pathfinding` 2D 网格 A* 与视线平滑寻路引擎（5/5 PASS，文档直写 `doc/lib/pathfinding.md`）

- **落地成果**：`lib/pathfinding/`（`grid.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ecs`（Sparse-Set 架构世代防 ABA 实体组件系统 ECS 引擎），与 `lib/barcode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:billing` SaaS 用量计费、周期折算与发票引擎（5/5 PASS，文档直写 `doc/lib/billing.md`）

- **落地成果**：`lib/billing/`（`billing.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:pathfinding`（2D 加权网格 A* 防穿角寻路、Dijkstra 距离场与 Bresenham 视线平滑引擎），与 `lib/barcode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ledger` 复式记账双分录会计总账引擎（5/5 PASS，文档直写 `doc/lib/ledger.md`）

- **落地成果**：`lib/ledger/`（`ledger.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:barcode`（Code 128 Auto A/B/C 与 EAN-13/UPC-A 一维条码编码、校验位计算及矢量 SVG/ASCII 渲染引擎），与 `lib/billing/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sitemap` RFC 9309 `robots.txt` 与 XML Sitemap 引擎（5/5 PASS，文档直写 `doc/lib/sitemap.md`）

- **落地成果**：`lib/sitemap/`（`sitemap.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:billing`（SaaS & AI API 阶梯/累进用量计费、周期按比例折算 Proration、Coupon 折扣与发票生成引擎），与 `lib/ledger/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:lz4` 纯 CP LZ4 Block & Frame 压缩引擎（5/5 PASS，文档直写 `doc/lib/lz4.md`）

- **落地成果**：`lib/lz4/`（`lz4.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:ledger`（复式记账双分录会计总账、不可变冲正与试算平衡对账引擎），与 `lib/sitemap/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:mermaid` 声明式 Mermaid 图表构建引擎（5/5 PASS，文档直写 `doc/lib/mermaid.md`）

- **落地成果**：`lib/mermaid/`（`diagram.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sitemap`（RFC 9309 `robots.txt` 规则匹配与 XML Sitemap 0.9 / Sitemap Index 解析生成引擎），与 `lib/lz4/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bplus` B+ 树有序键值索引与范围扫描引擎（5/5 PASS，文档直写 `doc/lib/bplus.md`）

- **落地成果**：`lib/bplus/`（`bplus.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:lz4`（纯 CP LZ4 Block & Frame 极速压缩解压与 XXH32 校验引擎），与 `lib/mermaid/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bencode` BEP-3 Bencode 编解码与 Torrent InfoHash 引擎（5/5 PASS，文档直写 `doc/lib/bencode.md`）

- **落地成果**：`lib/bencode/`（`bencode.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:mermaid`（声明式 Flowchart / Sequence / ER / State Mermaid 图表构建与安全转义引擎），与 `lib/bplus/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:feed` RSS 2.0 / Atom 1.0 / JSON Feed 1.1 统一引擎（5/5 PASS，文档直写 `doc/lib/feed.md`）

- **落地成果**：`lib/feed/`（`feed.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bplus`（B+ 树有序键值索引、节点分裂合并与叶子链表高效 `rangeScan`/`prefixScan` 引擎），与 `lib/bencode/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:selector` CSS3 选择器编译器与 DOM 查询引擎（5/5 PASS，文档直写 `doc/lib/selector.md`）

- **落地成果**：`lib/selector/`（`selector.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bencode`（BitTorrent BEP-3 Bencode 严格编解码、规范字典序校验与 SHA-1 `InfoHash` 提取引擎），与 `lib/feed/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:optimize` 数值求根与多维函数最优化引擎（5/5 PASS，文档直写 `doc/lib/optimize.md`）

- **落地成果**：`lib/optimize/`（`opt.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:feed`（RSS 2.0 / Atom 1.0 / JSON Feed 1.1 自动识别、统一解析与生成引擎），与 `lib/selector/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:quadtree` 2D 空间四叉树索引与 KNN 引擎（5/5 PASS，文档直写 `doc/lib/quadtree.md`）

- **落地成果**：`lib/quadtree/`（`quadtree.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:optimize`（数值求根 Brent/Bisection、黄金分割 1D 极小化、Nelder-Mead 无导数多维最优化与 Adam 梯度优化器），与 `lib/selector/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:prob` 概率分布、随机采样与 A/B 显著性检验引擎（5/5 PASS，文档直写 `doc/lib/prob.md`）

- **落地成果**：`lib/prob/`（`dist.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:selector`（CSS3 选择器解析与 XML/HTML DOM 树查询匹配引擎），与 `lib/quadtree/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:geom2d` 2D 计算几何、凸包与碰撞检测引擎（5/5 PASS，文档直写 `doc/lib/geom2d.md`）

- **落地成果**：`lib/geom2d/`（`geom.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:quadtree`（2D 空间四叉树索引、矩形/圆形范围检索与 K 近邻查询引擎），与 `lib/prob/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:fft` Radix-2 FFT/IFFT 与频谱分析引擎（5/5 PASS，文档直写 `doc/lib/fft.md`）

- **落地成果**：`lib/fft/`（`fft.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/linalg/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:geom2d`（2D 计算几何、Andrew 凸包、线段相交与碰撞检测引擎），与 `lib/prob/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:linalg` 通用稠密矩阵与数值线性代数引擎（5/5 PASS，文档直写 `doc/lib/linalg.md`）

- **落地成果**：`lib/linalg/`（`matrix.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:prob`（概率分布 PDF/CDF/分位数、蒙特卡洛采样与 A/B 测试显著性 Z-test/t-test 引擎），与 `lib/fft/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bpe` Byte-Level BPE Tokenizer 引擎（5/5 PASS，文档直写 `doc/lib/bpe.md`）

- **落地成果**：`lib/bpe/`（`bpe.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/regex/` 合并提交，并以 `Model: "inherit"` 顶替派发 `cap:fft`（Cooley-Tukey Radix-2 FFT/IFFT、窗函数、频谱功率与主频提取 DSP 引擎），与 `lib/linalg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:regex` 抗 ReDoS 线性时间 Pike VM 正则引擎（5/5 PASS，文档直写 `doc/lib/regex.md`）

- **落地成果**：`lib/regex/`（`compiler.cp`/`vm.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:linalg`（通用任意维度稠密矩阵运算、列主元 LU/高斯消元求解、求逆、行列式与最小二乘线性拟合引擎），与 `lib/bpe/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:svg` 矢量 SVG 画布与统计图表引擎（5/5 PASS，文档直写 `doc/lib/svg.md`）

- **落地成果**：`lib/svg/`（`canvas.cp`/`charts.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bpe`（Byte-Level BPE Tokenizer 训练、合并规则编码、字节级无损解码与 Special Tokens 隔离引擎），与 `lib/regex/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:sketch` HLL / T-Digest / 蓄水池概率素描引擎（5/5 PASS，文档直写 `doc/lib/sketch.md`）

- **落地成果**：`lib/sketch/`（`sketch.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:regex`（抗 ReDoS 线性时间 Pike VM / Thompson NFA 正则表达式编译器、捕获组与替换分割引擎），与 `lib/svg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:intern` 字符串驻留池与双向映射表引擎（5/5 PASS，文档直写 `doc/lib/intern.md`）

- **落地成果**：`lib/intern/`（`intern.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:sketch`（HyperLogLog 基数估算、流式 T-Digest/质心分位数与蓄水池等概率抽样引擎），与 `lib/svg/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ical` RFC 5545 iCalendar 与 RRULE 重复规则引擎（5/5 PASS，文档直写 `doc/lib/ical.md`）

- **落地成果**：`lib/ical/`（`ical.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:svg`（零 C 依赖矢量 SVG 绘图画布与折线/柱状/饼图数据可视化引擎），与 `lib/intern/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:zset` 跳表 + 哈希双索引有序集合引擎（5/5 PASS，文档直写 `doc/lib/zset.md`）

- **落地成果**：`lib/zset/`（`zset.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5，零内存泄漏），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:intern`（高性能字符串驻留池 `StringInterner` + 双向唯一映射表 `BiMap` 引擎），与 `lib/ical/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:raft` 确定性 Raft 共识核心状态机（5/5 PASS，文档直写 `doc/lib/raft.md`）+ 热日志冷归档

- **落地成果**：`lib/raft/`（`raft.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，同时将 `CHANGELOG.md`/`DEVLOG.md` 早期记录安全滚动归档至 `*_H.md`；以 `Model: "inherit"` 顶替派发 `cap:ical`（RFC 5545 iCalendar `.ics` 解析生成与 `RRULE` 重复规则展开引擎），与 `lib/zset/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:agentmem` AI Agent 分层长期记忆与召回引擎（5/5 PASS，文档直写 `doc/lib/agentmem.md`）

- **落地成果**：`lib/agentmem/`（`memory.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:zset`（跳表 SkipList + 哈希双索引有序集合引擎：`zadd`/`zrank`/`zrangeByScore`/`zremRangeByScore` 等），与 `lib/raft/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:mime` MediaType 解析、`Accept` 协商与 `MimeDb` 引擎（5/5 PASS，文档直写 `doc/lib/mime.md`）

- **落地成果**：`lib/mime/`（`mime.cp`/`db.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:raft`（确定性无 I/O Raft 共识核心状态机：任期选举、日志新旧校验、多数派提交与防脑裂退位），与 `lib/agentmem/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:jsonpath` RFC 9535 JSONPath 编译器与查询引擎（5/5 PASS，文档直写 `doc/lib/jsonpath.md`）

- **落地成果**：`lib/jsonpath/`（`parser.cp`/`query.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:agentmem`（AI Agent 分层长期记忆引擎：Working/Episodic/Semantic 三层记忆、三因子召回评分与巩固遗忘机制），与 `lib/mime/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:lock` 读写锁、死锁检测与 Fencing 租约锁引擎（5/5 PASS，文档直写 `doc/lib/lock.md`）

- **落地成果**：`lib/lock/`（`wait_graph.cp`/`lock.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:mime`（RFC 6838 MediaType 解析、HTTP `Accept` 内容协商与 120+ 扩展名双向映射引擎），与 `lib/jsonpath/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:health` 健康探针与优雅停机协调器（5/5 PASS，文档直写 `doc/lib/health.md`）

- **落地成果**：`lib/health/`（`probe.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:jsonpath`（RFC 9535 JSONPath 递归下降、数组切片与过滤表达式查询引擎），与 `lib/lock/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pool` 通用有界资源与连接池引擎（5/5 PASS，文档直写 `doc/lib/pool.md`）

- **落地成果**：`lib/pool/`（`pool.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:lock`（进程内按名逻辑读写锁、等待图死锁检测与 TTL/Fencing Token 租约锁引擎），与 `lib/health/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:bitset` 动态位图与零堆固定位图引擎（5/5 PASS，文档直写 `doc/lib/bitset.md`）

- **落地成果**：`lib/bitset/`（`bitset.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:health`（云原生 Liveness/Readiness/Startup 健康探针汇聚与优雅停机 `ShutdownCoordinator` 引擎），与 `lib/pool/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:pipeline` 洋葱模型中间件与多阶段流水线引擎（5/5 PASS，文档直写 `doc/lib/pipeline.md`）

- **落地成果**：`lib/pipeline/`（`pipeline.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），与 `lib/uritemplate/` 合并收尾提交，并以 `Model: "inherit"` 顶替派发 `cap:pool`（通用资源与连接池引擎：空闲/最大生命周期驱逐、健康校验与泄漏观测），与 `lib/bitset/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:uritemplate` RFC 6570 URI Template 展开引擎（5/5 PASS，文档直写 `doc/lib/uritemplate.md`）

- **落地成果**：`lib/uritemplate/`（`template.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:bitset`（字级 64-bit 向量化动态位图、硬件 Popcount/CTZ 快速迭代与集合布尔/Jaccard 运算引擎），与 `lib/pipeline/` 保持 2 路正交并行。

---

## 2026-11-21: 新建 `cap:ringbuf` 环形缓冲、字节环与微批聚合引擎（5/5 PASS，文档直写 `doc/lib/ringbuf.md`）

- **落地成果**：`lib/ringbuf/`（`ring.cp`/`index.cp`）经 `verify_module.sh` 一键全绿通过（5/5），完成回收与提交，并以 `Model: "inherit"` 顶替派发 `cap:pipeline`（可组合中间件洋葱链、短路拦截与多阶段可观测执行管道），与 `lib/uritemplate/` 保持 2 路正交并行。

---

