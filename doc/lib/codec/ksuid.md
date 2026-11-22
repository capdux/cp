# `cap:ksuid` — Segment KSUID 20 字节 / 27 字符 Base62 字典序唯一 ID 标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 1. 概述
`cap:ksuid`（动态挂载 `cap.ksuid`）是纯 CP 零堆分配实现的 Segment KSUID（4 字节相对时间戳 + 16 字节 CSPRNG 随机载荷 = 20 字节 -> 27 字符 Base62 固定宽度编码），支持按时间字典序排序、构造/反解与零堆编解码。

## 2. 核心类型
- `KsuidError`：`InvalidLength`、`InvalidCharacter`、`Overflow`、`TimestampOutOfRange`
- `Ksuid`（`nil`/`fromBytes`/`fromParts`/`generate`/`generateAt`/`timestampUnix`/`payload`/`encode`/`decode`/`compare`/`equals`）

## 3. API 速查
- `Ksuid.generate()` / `Ksuid.generateAt(unix_sec)` / `Ksuid.fromParts(unix_sec, payload)`
- `id.encode()` -> `[27]u8` / `Ksuid.decode(str)` -> `Ksuid`

## 4. 示例
```cp
import { Ksuid } from "cap:codec";
```
