# `cap:bvh` — Biovision Hierarchy (`.bvh`) 3D 骨骼动捕零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

> **导入方式**：`import { BvhChannel, parseBvhChannel, isBvh, BvhJointIter, parseBvhHeader, BvhFrameIter, BvhFloatIter } from "cap:gfx";`  
> **内存模型**：100% 纯 CP 零堆分配，关节名与每帧切片均零拷贝指向原始 BVH 缓冲区。

---

## 核心类型

- **`BvhChannel: u8`**：`x_pos=0`, `y_pos=1`, `z_pos=2`, `x_rot=3`, `y_rot=4`, `z_rot=5`, `unknown=255`
- **`BvhJoint`**：`name`, `depth`, `is_root`, `is_end_site`, `offset_x`/`offset_y`/`offset_z`, `channel_count`, `channels: [6]BvhChannel`, `channel_offset: u32`
- **`BvhHeader`**：`joint_count`, `end_site_count`, `total_channels`, `frame_count`, `frame_time_sec`, `frames_offset`，提供 `fps(): f64` 与 `durationSec(): f64`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parseBvhChannel` | `fn parseBvhChannel(name: []const u8): BvhChannel` | 解析通道名（如 `Xposition`/`Zrotation`） |
| `isBvh` | `fn isBvh(data: []const u8): bool` | 跳过前导空白后检测 `"HIERARCHY"` 头 |
| `BvhJointIter` | `BvhJointIter.init(data)` / `it.next(): BvhJoint?` | 深度优先遍历 `ROOT`/`JOINT`/`End Site`，追踪 `depth` 与 `channel_offset` |
| `parseBvhHeader` | `fn parseBvhHeader(data: []const u8): BvhHeader?` | 扫描 `HIERARCHY` 与 `MOTION` 头，校验花括号匹配与 `Frames`/`Frame Time` |
| `BvhFrameIter` | `BvhFrameIter.init(data, header)` / `it.next(): ([]const u8)?` | 按行产出每帧文本切片，校验通道浮点计数等于 `total_channels` |
| `BvhFloatIter` | `BvhFloatIter.init(frame_line)` / `it.next(): f64?` | 对单帧切片逐个解码 `f64` 通道值（支持科学计数法） |

---

## 使用示例

```typescript
import { parseBvhHeader, BvhJointIter, BvhFrameIter, BvhFloatIter } from "cap:gfx";

fn inspectBvh(bvh_bytes: []const u8) {
    if (hdr := parseBvhHeader(bvh_bytes)) {
        * = hdr.fps();
        let fit = BvhFrameIter.init(bvh_bytes, hdr);
        while (frame := fit.next()) {
            let vals = BvhFloatIter.init(frame);
            while (v := vals.next()) {
                * = v;
            }
        }
    }
}
```
