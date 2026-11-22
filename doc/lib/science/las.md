# `cap:las` — 零分配 ASPRS LAS 1.2–1.4 激光雷达 3D 点云解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

> **模块路径**：`lib/las/`（导入方式：`import { ... } from "cap:science";`）  
> **特性**：100% 纯 CP、零堆分配、完整支持 LAS 1.2/1.3/1.4 Public Header Block、LAZ 压缩标志识别、VLR 变长记录与 Format 0–10 点云坐标缩放解码。

---

## 一、 核心常量

| 常量名 | 值 | 说明 |
| :--- | :--- | :--- |
| `LAS_MAGIC` | `"LASF"` | 4 字节 LAS 文件签名 |
| `LAS_HEADER_SIZE_1_2` | `227` | LAS 1.2 Public Header 最小字节数 |
| `LAS_HEADER_SIZE_1_3` | `235` | LAS 1.3 Public Header 字节数 |
| `LAS_HEADER_SIZE_1_4` | `375` | LAS 1.4 Public Header 字节数（含 64-bit 扩展点数） |
| `LAS_VLR_HEADER_SIZE` | `54` | VLR 变长记录头部固定字节数 |

---

## 二、 核心 API

- `isLas(data)` / `parseLasHeader(data): LasHeader?`  
  解析版本、系统/软件标识、偏移、缩放因子、包围盒、LAZ 压缩位（`raw_fmt & 0x80`）与点数（v1.4 自动取偏移 247 的 64-bit `extended_point_count`）。
- `LasVlrIter.init(data, header)` / `next(*_): LasVlr?`  
  从 `header_size` 迭代 `num_vlrs` 条 VLR，严格限制在 `offset_to_point_data` 之前。
- `LasPointIter.init(data, header)` / `next(*_): LasPoint?`  
  按 `raw * scale + offset` 换算三维坐标并解码强度、回波数与分类码（兼容 Format 0–5 与 Format 6–10）；检测到 LAZ 压缩时返回空迭代器。

---

## 三、 使用示例

```typescript
import { parseLasHeader, LasVlrIter, LasPointIter } from "cap:science";

fn inspectLas(bytes: []const u8) {
    if (hdr := parseLasHeader(bytes)) {
        let vlrs = LasVlrIter.init(bytes, hdr);
        while (v := vlrs.next()) {
            * = v.record_id;
        }
        let pts = LasPointIter.init(bytes, hdr);
        while (pt := pts.next()) {
            * = pt.z;
        }
    }
}
```
