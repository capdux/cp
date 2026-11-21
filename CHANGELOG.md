# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:node_builtins` 纯 CP Node.js & Bun 内建模块映射与规范化注册表（吸收自 Bun `src/resolve_builtins/HardcodedModule.cp`，支持 `cap.node_builtins` 自动动态挂载，详见 `doc/lib/node_builtins.md`）（5/5 PASS，自动动态挂载 `cap.node_builtins`）
- **新增标准库**：`lib/node_builtins/{builtins,index}.cp`，从 Bun `src/resolve_builtins/HardcodedModule.cp` 吸收完整 `node:*`、裸模块、历史别名与 Jest/Vitest 重写规则。\n- **新增文档与测试**：`doc/lib/node_builtins.md` 扁平文档与 `cap/test/test_node_builtins_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:http_types` 纯 CP HTTP 核心类型、ETag 弱匹配、Fetch 枚举与零堆 URLPath 解析引擎（吸收自 Bun `src/http_types/*.cp`，支持 `cap.http_types` 自动动态挂载，详见 `doc/lib/http_types.md`）（5/5 PASS，自动动态挂载 `cap.http_types`）
- **新增标准库**：`lib/http_types/{http_types,index}.cp`，从 Bun `src/http_types/` 吸收 `Method`（36 种方法）、`Encoding`、Fetch 枚举、`ETag` 与 `URLPath`。\n- **新增文档与测试**：`doc/lib/http_types.md` 扁平文档与 `cap/test/test_http_types_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:union_interval` 纯 CP 不相交区间合并集合引擎（自动合并重叠/相邻区间、删除自动分裂与 $O(\log N)$ 二分点查，支持 `cap.union_interval` 自动动态挂载，详见 `doc/lib/union_interval.md`）（5/5 PASS，自动动态挂载 `cap.union_interval`）
- **新增标准库**：`lib/union_interval/{interval_set,index}.cp`，实现泛型半开区间合并集合 `IntervalSet(T)`。\n- **新增文档与测试**：`doc/lib/union_interval.md` 扁平文档与 `cap/test/test_union_interval_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:varint_leb128` 纯 CP 零堆 ULEB128 / SLEB128 / ZigZag 32/64 位变长整数编解码引擎（支持 10 字节溢出与截断防护，支持 `cap.varint_leb128` 自动动态挂载，详见 `doc/lib/varint_leb128.md`）（5/5 PASS，自动动态挂载 `cap.varint_leb128`）
- **新增标准库**：`lib/varint_leb128/{varint,index}.cp`，实现 ULEB128、SLEB128 与 ZigZag 32/64 编解码。\n- **新增文档与测试**：`doc/lib/varint_leb128.md` 扁平文档与 `cap/test/test_varint_leb128_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:decimal_fp` 纯 CP 金融定点数 Decimal 精确算术引擎（`i128` 尾数 + `0..18` 标度，零浮点漂移、4 种舍入模式与字符串往返，支持 `cap.decimal_fp` 自动动态挂载，详见 `doc/lib/decimal_fp.md`）（5/5 PASS，自动动态挂载 `cap.decimal_fp`）
- **新增标准库**：`lib/decimal_fp/{decimal,index}.cp`，实现精确金融定点数 `Decimal` 四则运算与舍入。\n- **新增文档与测试**：`doc/lib/decimal_fp.md` 扁平文档与 `cap/test/test_decimal_fp_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:base64url` 纯 CP RFC 4648 Base64 与 JWT/JWK 专用无填充/自适应 Base64URL 编解码引擎（支持 `cap.base64url` 自动动态挂载，详见 `doc/lib/base64url.md`）（5/5 PASS，自动动态挂载 `cap.base64url`）
- **新增标准库**：`lib/base64url/{base64url,index}.cp`，实现 Standard Base64 与 Base64URL 编解码。\n- **新增文档与测试**：`doc/lib/base64url.md` 扁平文档与 `cap/test/test_base64url_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:bitset_simd` 纯 CP 按字向量化加速动态位集合引擎（基于 `u64` 字块 `@popCount`/`@ctz` 批量位运算、`setRange`/`nextSetBit` 及集合并交差，支持 `cap.bitset_simd` 自动动态挂载，详见 `doc/lib/bitset_simd.md`）（5/5 PASS，自动动态挂载 `cap.bitset_simd`）
- **新增标准库**：`lib/bitset_simd/{bitset,index}.cp`，实现字块加速 `BitSet` 位集与稀疏迭代器。\n- **新增文档与测试**：`doc/lib/bitset_simd.md` 扁平文档与 `cap/test/test_bitset_simd_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

