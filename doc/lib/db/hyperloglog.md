# `cap:hyperloglog` — 纯 CP HyperLogLog 基数估算引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 概述

`cap:hyperloglog`（亦支持 `cap.hyperloglog` 动态挂载）实现 64 位 Wyhash + HyperLogLog 基数估算器，支持 `4..18` 位精度配置、小规模 LinearCounting 线性修正、无损 `merge` 聚合及二进制序列化/反序列化。

## 核心类型与 API

- `HyperLogLog.init(alloc, precision)` / `deinit(&self, alloc)`
- `addBytes(&self, data)` / `addU64(&self, val)` / `count(self) -> u64`
- `merge(&self, other)` / `clear(&self)`
- `serialize(self, alloc)` / `deserialize(alloc, data)`

## 示例

```cp
import { HyperLogLog, DEFAULT_PRECISION } from "cap:db";

let hll = try HyperLogLog.init(alloc, DEFAULT_PRECISION);
defer hll.deinit(alloc);
hll.addBytes("user:1001");
const est = hll.count();
```
