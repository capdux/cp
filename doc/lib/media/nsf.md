# `cap:nsf` — NES Sound Format (`.nsf` / `.nsfe`) 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

> **导入方式**：`import { detectNsfFormat, isNsf, isNsfe, parseNsfHeader, NsfeChunkIter, findNsfeChunk, parseNsfeInfo, NSF_CHIP_VRC6 } from "cap:media";`  
> **内存模型**：100% 纯 CP 零堆分配，曲名/作者/版权、6502 程序区与 NSFe 分块负载均零拷贝指向原始缓冲区。

---

## 核心常量与枚举

- `NSF_MAGIC = "NESM\x1a"`、`NSF_HEADER_SIZE = 128`、`NSFE_MAGIC = "NSFE"`
- 扩展芯片位掩码：`NSF_CHIP_VRC6`、`NSF_CHIP_VRC7`、`NSF_CHIP_FDS`、`NSF_CHIP_MMC5`、`NSF_CHIP_NAMCO163`、`NSF_CHIP_SUNSOFT5B`
- `NsfRegion: u8` (`ntsc=0`, `pal=1`, `dual=2`, `unknown=255`)、`NsfFormat: u8` (`unknown=0`, `nsf=1`, `nsfe=2`)

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `detectNsfFormat` / `isNsf` / `isNsfe` | `fn detectNsfFormat(data): NsfFormat` | 识别 NSF 128B 与 NSFe 容器 |
| `NsfHeader` / `parseNsfHeader` | `fn parseNsfHeader(data): NsfHeader?` | 解析 128B NSF 1.0/2.0 头，提供 `usesBankswitching()`, `hasChip(mask)`, `ntscFps()`, `palFps()` |
| `NsfeChunk` / `NsfeChunkIter` | `NsfeChunkIter.init(data)` / `next(): NsfeChunk?` | 零分配遍历 NSFe 分块直至 `"NEND"`，防越界溢出 |
| `findNsfeChunk` | `fn findNsfeChunk(data, id_4b): ([]const u8)?` | 按 4B ID 查找分块负载 |
| `NsfeInfo` / `parseNsfeInfo` | `fn parseNsfeInfo(info_payload): NsfeInfo?` | 解析 `"INFO"` 块（兼容 8B 最小格式与 10B 完整格式） |

---

## 使用示例

```typescript
import { parseNsfHeader, NSF_CHIP_VRC6 } from "cap:media";

fn inspectNsf(bytes: []const u8) {
    if (hdr := parseNsfHeader(bytes)) {
        * = hdr.song_name;
        * = hdr.hasChip(NSF_CHIP_VRC6);
        * = hdr.ntscFps();
    }
}
```
