# CP 标准库 SIMD 性能基准与全量回归报告

## 1. 测试环境与编译基线

| 维度 | 配置详情 |
| --- | --- |
| **CPU 架构** | Intel(R) Xeon(R) Platinum 8175M CPU @ 2.50GHz (x86_64, 2 Sockets / 48 Cores / 96 Threads, SSE4.2 / AVX2 / AVX-512 / POPCNT) |
| **操作系统** | Linux x86_64 (`CLOCK_MONOTONIC` 纳秒级单调硬件计时器 `HwTimer.nowNs()`) |
| **编译器工具链** | `cpc` (CP 前端编译器) + Zig `0.17.0` |
| **基准构建模式** | `zig build-exe -OReleaseFast`（吞吐实测）& `cap test` Debug（0 差异回归门禁） |
| **基准源码** | [`cap/test/benchSimdStdlib.cp`](../cap/test/benchSimdStdlib.cp)（`cpc brief` 4 门禁全 0：无 `zig{}` / 无 `var` / 无 `tuple.@N` / 无 `anyopaque`） |

### 可复现执行命令

```bash
# 1. Debug 模式 0 差异回归与快速验证（~1.5s）
cap test --cache-dir=.cap/ws_t2_bench cap/test/benchSimdStdlib.cp

# 2. ReleaseFast 原生吞吐与加速比实测
mkdir -p temp/bench_simd && cp .cap/zig/cap/test/benchSimdStdlib.zig temp/bench_simd/
(cd temp/bench_simd && zig build-exe -OReleaseFast benchSimdStdlib.zig && ./benchSimdStdlib)
rm -rf temp/bench_simd
```

---

## 2. 八大代表性标准库 SIMD 热路径实测结果（ReleaseFast）

所有基准均包含**严格结果 0 差异校验**（整型/布尔逐位一致；浮点相对误差 `< 1e-4` / `< 1e-11`）与**防 DCE（死代码消除）累加校验和**（`std.mem.doNotOptimizeAway` + 迭代偏移扰动防 LICM 外提）。

| # | 模块与热路径内核 | 向量宽度 | 单轮载荷 × 迭代 | 标量耗时 | 标量吞吐 | SIMD 耗时 | SIMD 吞吐 | 加速比 | 0 差异校验 |
| :-: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | :-: |
| 1 | `cap:xml` (`simdFindXmlTextDelim` + `simdHasInvalidXmlControl`) | 16-lane `u8` | 16 KB × 1,024 (2×scan) | 24.617 ms | 1,299.9 MB/s | 3.457 ms | 9,255.6 MB/s | **7.12x** | PASS (0-diff) |
| 2 | `cap:ds` Roaring (`simdBitmapAnd` + `simdBitmapOr` + `simdBitmapPopcount`) | 4-lane `u64` | 32 KB (4,096×u64) × 1,024 | 8.375 ms | 26,746.7 MB/s | 8.434 ms | 26,559.5 MB/s | **0.99x** | PASS (0-diff) |
| 3 | `cap:ds` HyperLogLog (`simdHllMergeRegisters` `@max`) | 16-lane `u8` | 16 KB (`p=14`) × 2,048 | 22.067 ms | 4,350.5 MB/s | 1.650 ms | 58,180.2 MB/s | **13.37x** | PASS (0-diff) |
| 4 | `cap:vector` HNSW/IVF-PQ (`simdVecIdxL2SqF32` + `simdVecIdxDotF32`) | 4-lane `f32` | 1,536-dim × 4,096 (2×kernel) | 16.256 ms | 774.1 Mops/s | 3.974 ms | 3,166.4 Mops/s | **4.09x** | PASS (<1e-5) |
| 5 | `cap:math` StatsDist (`simdMathDotF64` + `simdMathSumSqDiffF64`) | 4-lane `f64` | 4,096-dim × 2,048 (2×kernel) | 21.983 ms | 763.2 Mops/s | 8.755 ms | 1,916.3 Mops/s | **2.51x** | PASS (<1e-11) |
| 6 | `cap:net` ICMP RFC 1071 (`simdIcmpChecksumFold`) | 8-lane `u32` | 16 KB × 2,048 | 8.596 ms | 3,722.6 MB/s | 2.185 ms | 14,645.8 MB/s | **3.93x** | PASS (0-diff) |
| 7 | `cap:auth` 常量时间比对 (`simdAuthCtEqual`, 无早退) | 16-lane `u8` | 4 KB × 4,096 (eq+ne) | 13.089 ms | 4,889.4 MB/s | 0.993 ms | 64,421.3 MB/s | **13.18x** | PASS (0-diff) |
| 8 | `cap:text` Myers Diff (`simdCommonPrefixLen` + `simdFindNewline`) | 16-lane `u8` | 16 KB × 2,048 (2×scan) | 45.177 ms | 2,125.0 MB/s | 3.629 ms | 26,454.2 MB/s | **12.45x** | PASS (0-diff) |

### 关键性能洞察

1. **16-lane 字节流扫描与无分支归约（7.12x ~ 13.37x）**：`cap:ds` HLL `@max` 归并（**13.37x**，58.2 GB/s）、`cap:auth` 常量时间无早退比对（**13.18x**，64.4 GB/s）、`cap:text` Myers 公共前缀与换行扫描（**12.45x**，26.5 GB/s）以及 `cap:xml` 文本分隔符/控制字符扫描（**7.12x**，9.3 GB/s）均充分释放了 128-bit 向量寄存器吞吐，彻底消除逐字节分支预测惩罚。
2. **浮点向量核逼近理论带宽（2.51x ~ 4.09x）**：`cap:vector` 4-lane `f32` L2/内积达到 **4.09x**（3.17 Gops/s），紧贴 4-lane 理论上限；`cap:math` 4-lane `f64` 统计核在保留逐元素 `isFinite` NaN/Inf 安全守卫下仍录得 **2.51x**（1.92 Gops/s）；`cap:net` 8-lane RFC 1071 一补码折叠达到 **3.93x**（14.6 GB/s）。
3. **Roaring 4-lane `u64` 位图核的内存带宽天花板现象（0.99x）**：在 x86_64 `-OReleaseFast` 下，32 KB 位图连续执行 AND/OR/Popcount 合计 7 路读写已触达 L1/L2 缓存带宽上限（~26.7 GB/s），且标量 `@popCount(u64)` 已被 LLVM 自动展开并调度至硬件 `POPCNT` 流水线，因此显式 4-lane `@Vector(4, u64)` 与标量实现持平（26.56 GB/s vs 26.75 GB/s），同时在非自动向量化目标平台上仍保证确定性向量语义。
4. **Debug 回归验证**：`cap test --cache-dir=.cap/ws_t2_bench cap/test/benchSimdStdlib.cp` **8/8 PASS**（总耗时 **1,501 ms**），确保 CI 阶段即可零成本守卫全部 8 个热路径的 SIMD/标量等价性。

---

## 3. 全量回归验证结果

- **测试范围**：全仓全部 **101 个** `cap/test/test*RustRefSuite.cp` 深度加固回归套件（独立缓存 `--cache-dir=.cap/ws_t2_regress/<suite>`，4 路并行执行）。
- **最终结果**：**101 / 101 PASS (100%)**（总耗时 ~76s；终验 4 路并行复跑 5s）。
- **首轮拦截并顺手修复的 4 项真实回归/规范边界（全部通过 `cpc brief` 4 门禁全 0，零删减断言）**：
  1. `lib/doc/csvTable.cp` + `cap/test/testDocCsvRustRefSuite.cp`：修复 `u64` 位移字面量 `1 << 63` 与入口包装（9/9 PASS）；
  2. `lib/image/jpeg.cp` + `cap/test/testImageJpegRustRefSuite.cp`：补齐非交错多扫描 Baseline SOS 与 ITU-T T.81 §A.2.2 单分量 1×1 MCU 规范（10/10 PASS）；
  3. `lib/image/webp.cp` + `cap/test/testImageWebpRustRefSuite.cp`：修复 `[4]u8` 字面量、`ArrayList.empty` 与 `dark.webp` 有损 VP8 校验（16/16 PASS）；
  4. `lib/pack/zstdDecode.cp` + `cap/test/testPackZstdRustRefSuite.cp`：按 RFC 8878 修复小帧 `Single_Segment` 压缩块大小下限 1024B（6/6 PASS）。
