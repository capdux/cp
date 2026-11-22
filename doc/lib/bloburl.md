# `cap:bloburl` — 零堆分配 W3C Blob Object URL 解析与注册表库

`cap:bloburl` 吸收自 Bun 上游 WebCore Object URL 注册表实现（`src/runtime/webcore/ObjectURLRegistry.cp`），剥离了 JSC 与全局锁堆表耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 `blob:` Object URL 解析、格式化、RFC 4122 UUID v4 生成以及固定容量注册/解析/吊销注册表（`BlobUrlRegistry`）。

## 核心特性

- **100% 零堆分配**：注册表槽位池（`[]BlobUrlEntry`）与 URL 输出缓冲（`[]u8`）均由调用方提供；内置无全局状态 SplitMix64 确定性 UUID v4 生成器。
- **双规范格式兼容**：同时支持 Bun 原生无 origin 的 `blob:<uuid>`（41 字节）与 W3C File API 带 origin 的 `blob:<origin>/<uuid>`（如 `blob:https://example.com/<uuid>`）。
- **RFC 4122 UUID v4 精确编解码**：自动设置 version (`0x4x`) 与 variant (`10xx`) 位，支持大小写混合十六进制解析与规范小写 36 字符格式化。
- **幂等生命周期管理**：`createObjectUrl`、`resolve`、`has`、`revoke` 与槽位复用零泄露；注册表满或缓冲不足时原子返回 `null`。

---

## 导出清单

### 常量与结构体

- `BLOB_PREFIX = "blob:"`、`UUID_STR_LEN = 36`、`BLOB_URL_MIN_LEN = 41`
- `BlobUuid`：`{ bytes: [16]u8 }`，附 `eql`、`parse`、`format`
- `ParsedBlobUrl`：`{ origin, uuid }`
- `BlobUrlEntry`：`{ occupied, uuid, payload_id, content_type, size }`
- `BlobUrlRegistry`：`init`、`createObjectUrl`、`resolve`、`has`、`revoke`、`activeCount`

### 独立函数

| 函数签名 | 说明 |
| :--- | :--- |
| `isBlobUrl(url: []const u8): bool` | 快速前缀与最小长度校验（对应 Bun `isBlobURL`） |
| `parseBlobUrl(url: []const u8): ParsedBlobUrl?` | 解析 `blob:<uuid>` 或 `blob:<origin>/<uuid>` |
| `formatBlobUrl(origin, uuid, out_buf): usize?` | 格式化规范 `blob:` Object URL |

---

## 使用示例

```typescript
import std from "std";
import { BlobUrlEntry, BlobUrlRegistry } from "cap:bloburl";

fn blobUrlDemo() {
    let slots: [16]BlobUrlEntry = std.mem.zeroes([16]BlobUrlEntry);
    let slots_s: []BlobUrlEntry = slots[0..];
    let reg = BlobUrlRegistry.init(&slots_s, 42) ?? return;

    let url_buf: [128]u8 = std.mem.zeroes([128]u8);
    let url_s: []u8 = url_buf[0..];
    const n = reg.createObjectUrl(1001, "image/png", 4096, "https://app.local", &url_s) ?? return;

    const entry = reg.resolve(url_buf[0..n]);
    * = entry;
    * = reg.revoke(url_buf[0..n]);
}
```
