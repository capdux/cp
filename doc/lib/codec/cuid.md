# `cap:cuid` — 纯 CP 零分配 CUID / CUID2 校验、分类与确定性编码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 概述

`cap:cuid` 提供纯 CP、零外部依赖、零堆分配的 CUID v1 / CUID2 碰撞安全 ID 校验器、版本分类器、无符号 64 位小写 base36 编解码器，以及确定性 CUID v1 编码与时间戳提取工具。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `CuidVersion` | 版本枚举：`unknown`、`v1`、`v2` |
| `isValidCuid1(s)` | 校验 25 字节、`'c'` 开头、其余 24 字节全小写 base36 `[0-9a-z]` |
| `isValidCuid2(s)` | 校验长度 `2..=32`、首字节 `[a-z]`、其余全小写 base36 `[0-9a-z]` |
| `detectCuidVersion(s)` | 自动识别 `CuidVersion.v1` / `v2` / `unknown` |
| `encodeBase36(val, out)` | 零分配 `u64` 小写 base36 编码，缓冲不足返回 `null` |
| `decodeBase36(s)` | 零分配小写 base36 解码为 `u64`，拦截空串、非法字符与溢出 |
| `formatCuid1(ts, cnt, fp, rand, out)` | 按 25 字节定长结构 `'c' + ts(8) + cnt(4) + fp(4) + rand(8)` 确定性编码 |
| `parseCuid1Timestamp(s)` | 从合法 25 字节 CUID v1 提取毫秒级时间戳 |

## 示例

```cp
import { formatCuid1, parseCuid1Timestamp, detectCuidVersion, CuidVersion } from "cap:codec";

fn example(): bool {
    let buf: [32]u8 = undefined;
    if (id := formatCuid1(1700000000000, 42, "ab12", 987654321, buf[0..])) {
        const ts = parseCuid1Timestamp(id) ?? 0;
        return detectCuidVersion(id) == CuidVersion.v1 && ts == 1700000000000;
    }
    return false;
}
```
