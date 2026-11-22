# `cap:vgm` — Video Game Music (`.vgm`) 1.00–1.71 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

> **导入方式**：`import { isVgm, parseVgmHeader, parseVgmGd3, decodeUtf16LeToUtf8, scanVgmCommands } from "cap:media";`  
> **内存模型**：100% 纯 CP 零堆分配，GD3 标签切片零拷贝指向原始 VGM 缓冲区。

---

## 核心常量

- `VGM_MAGIC = "Vgm "`、`VGM_GD3_MAGIC = "Gd3 "`、`VGM_MIN_HEADER_SIZE = 64`、`VGM_SAMPLE_RATE = 44100`
- 命令字节：`CMD_WAIT_N = 0x61`、`CMD_WAIT_60HZ = 0x62`、`CMD_WAIT_50HZ = 0x63`、`CMD_END = 0x66`、`CMD_DATA_BLOCK = 0x67`、`CMD_WAIT_SHORT_START = 0x70`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isVgm` | `fn isVgm(data: []const u8): bool` | 校验长度 >= 64 且首 4B 为 `"Vgm "` |
| `VgmHeader` / `parseVgmHeader` | `fn parseVgmHeader(data): VgmHeader?` | 解析 VGM 1.00–1.71 头、BCD 版本号（`versionMajor()`/`versionMinor()`）、音源时钟、相对偏移与 `durationMs()` |
| `VgmGd3` / `parseVgmGd3` | `fn parseVgmGd3(data, header): VgmGd3?` | 解析 `"Gd3 "` 标签块并零拷贝切分 11 个 UTF-16LE 字段 |
| `decodeUtf16LeToUtf8` | `fn decodeUtf16LeToUtf8(utf16le, out): usize?` | 零堆分配将 UTF-16LE 解码为 UTF-8/ASCII |
| `VgmCmdStats` / `scanVgmCommands` | `fn scanVgmCommands(data, header): VgmCmdStats` | 单遍扫描命令区至 `CMD_END`，跳过 `0x67` Data Block 与各类芯片写命令，累计等待采样数 |

---

## 使用示例

```typescript
import { parseVgmHeader, parseVgmGd3, scanVgmCommands } from "cap:media";

fn inspectVgm(bytes: []const u8) {
    if (hdr := parseVgmHeader(bytes)) {
        * = hdr.durationMs();
        if (gd3 := parseVgmGd3(bytes, hdr)) {
            * = gd3.track_name_en;
        }
        const stats = scanVgmCommands(bytes, hdr);
        * = stats.computed_wait_samples;
    }
}
```
