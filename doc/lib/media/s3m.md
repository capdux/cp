# `cap:s3m` — Scream Tracker 3 (`.s3m`) 跟踪器音乐零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

> **导入方式**：`import { isS3m, parseS3mHeader, parseS3mInstrument, parseS3mPattern } from "cap:media";`  
> **内存模型**：100% 纯 CP 零堆分配，曲名、乐器名、Order 表与 Pattern 打包数据均零拷贝指向原始模块缓冲区。

---

## 核心常量

- `S3M_MAGIC = "SCRM"`、`S3M_HEADER_SIZE = 96`、`S3M_SUB_BYTE = 0x1A`、`S3M_TYPE_MODULE = 16`、`S3M_INST_SIZE = 80`、`S3M_ORDER_SKIP = 254`、`S3M_ORDER_END = 255`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isS3m` | `fn isS3m(data: []const u8): bool` | 校验 96B 头魔数（`0x1A`, `16`, `"SCRM"`） |
| `S3mHeader` / `parseS3mHeader` | `fn parseS3mHeader(data): S3mHeader?` | 解析 96B 头与三张表偏移，提供 `isStereo()`, `activeChannelCount()`, `ordersSlice(data)`, `instrumentOffset(data, idx)`, `patternOffset(data, idx)` |
| `S3mInstrument` / `parseS3mInstrument` | `fn parseS3mInstrument(data, header, idx): S3mInstrument?` | 解析 80B PCM/AdLib 乐器头，解码 24-bit `memseg * 16` 采样偏移与 `"SCRS"` 魔数 |
| `S3mPattern` / `parseS3mPattern` | `fn parseS3mPattern(data, header, idx): S3mPattern?` | 按 Parapointer 提取 Pattern 压缩负载切片 |

---

## 使用示例

```typescript
import { parseS3mHeader, parseS3mInstrument, parseS3mPattern } from "cap:media";

fn inspectS3m(bytes: []const u8) {
    if (hdr := parseS3mHeader(bytes)) {
        * = hdr.activeChannelCount();
        if (inst := parseS3mInstrument(bytes, hdr, 0)) {
            * = inst.title;
        }
        if (pat := parseS3mPattern(bytes, hdr, 0)) {
            * = pat.data.len;
        }
    }
}
```
