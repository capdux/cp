# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:node_builtins` 纯 CP Node.js & Bun 内建模块映射与规范化注册表（吸收自 Bun `src/resolve_builtins/HardcodedModule.cp`，支持 `cap.node_builtins` 自动动态挂载，详见 `doc/lib/node_builtins.md`）（5/5 PASS，文档直写 `doc/lib/node_builtins.md`）
- **吸收改造**：采用编译期 `std.StaticStringMap` 重构 `bun.ComptimeStringMap`，零堆分配实现目标隔离与前缀强制校验。

## 2026-11-21: 新建 `cap:http_types` 纯 CP HTTP 核心类型、ETag 弱匹配、Fetch 枚举与零堆 URLPath 解析引擎（吸收自 Bun `src/http_types/*.cp`，支持 `cap.http_types` 自动动态挂载，详见 `doc/lib/http_types.md`）（5/5 PASS，文档直写 `doc/lib/http_types.md`）
- **吸收改造**：去除 `bun` 宿主耦合，采用 `std.StaticStringMap` 与 `std.hash.XxHash64` 重写，通过全 5 组深度测试。

## 2026-11-21: 新建 `cap:union_interval` 纯 CP 不相交区间合并集合引擎（自动合并重叠/相邻区间、删除自动分裂与 $O(\log N)$ 二分点查，支持 `cap.union_interval` 自动动态挂载，详见 `doc/lib/union_interval.md`）（5/5 PASS，文档直写 `doc/lib/union_interval.md`）
- **核心设计**：线性归并合并重叠区间与二分点查，通过中段分裂/边界裁剪与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:varint_leb128` 纯 CP 零堆 ULEB128 / SLEB128 / ZigZag 32/64 位变长整数编解码引擎（支持 10 字节溢出与截断防护，支持 `cap.varint_leb128` 自动动态挂载，详见 `doc/lib/varint_leb128.md`）（5/5 PASS，文档直写 `doc/lib/varint_leb128.md`）
- **核心设计**：零堆流式位编码与符号扩展解码，通过 DWARF ULEB128/SLEB128 官方向量与溢出/EOF 拦截测试。

## 2026-11-21: 新建 `cap:decimal_fp` 纯 CP 金融定点数 Decimal 精确算术引擎（`i128` 尾数 + `0..18` 标度，零浮点漂移、4 种舍入模式与字符串往返，支持 `cap.decimal_fp` 自动动态挂载，详见 `doc/lib/decimal_fp.md`）（5/5 PASS，文档直写 `doc/lib/decimal_fp.md`）
- **核心设计**：采用 `i128` 尾数标度对齐算术与银行家舍入 `HalfEven`，通过 `"0.1"+"0.2"=="0.3"` 及溢出/除零拦截验证。

## 2026-11-21: 新建 `cap:base64url` 纯 CP RFC 4648 Base64 与 JWT/JWK 专用无填充/自适应 Base64URL 编解码引擎（支持 `cap.base64url` 自动动态挂载，详见 `doc/lib/base64url.md`）（5/5 PASS，文档直写 `doc/lib/base64url.md`）
- **核心设计**：封装 Zig 0.17 `std.base64` 标准与 URL 安全无填充编解码器，通过 RFC 4648 Section 10 向量与 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:bitset_simd` 纯 CP 按字向量化加速动态位集合引擎（基于 `u64` 字块 `@popCount`/`@ctz` 批量位运算、`setRange`/`nextSetBit` 及集合并交差，支持 `cap.bitset_simd` 自动动态挂载，详见 `doc/lib/bitset_simd.md`）（5/5 PASS，文档直写 `doc/lib/bitset_simd.md`）
- **核心设计**：按 64 位字块执行向量化位运算与 `@ctz` 跳位迭代，通过跨字边界与 `std.testing.allocator` 零泄漏验证。

