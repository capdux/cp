# `cap:uuid_v7` — 纯 CP RFC 9562 UUID v4 / v5 / 时间有序 v7 引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 概述

`cap:uuid_v7`（亦支持 `cap.uuid_v7` 动态挂载）实现 RFC 9562 零堆 UUID v4（CSPRNG）、UUID v5（SHA-1 命名空间确定性派生）与 UUID v7（48 位毫秒时间戳前缀、数据库 B-Tree 友好字典序递增）生成、格式化与解析。

## 核心类型与 API

- `UuidError`: `InvalidLength`、`InvalidFormat`、`InvalidHex`
- `NAMESPACE_DNS`、`NAMESPACE_URL`
- `Uuid`: `nil()`、`max()`、`v4()`、`v7()`、`v7At(unix_ms)`、`v5(namespace, name)`、`version()`、`variantRfc9562()`、`timestampMsV7()`、`format()`、`parse(s)`、`compare(other)`

## 示例

```cp
import { Uuid, NAMESPACE_DNS } from "cap:time";

const id = Uuid.v7();
const s = id.format();
const v5 = Uuid.v5(NAMESPACE_DNS, "example.com");
```
