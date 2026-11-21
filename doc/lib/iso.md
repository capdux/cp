# `cap:iso` — 零分配 ISO 9660 光盘镜像解析库

> **模块路径**：`lib/iso/`（导入方式：`import { ... } from "cap:iso";`）  
> **特性**：100% 纯 CP、零堆分配、零拷贝切片视图、全量 both-endian 一致性与越界溢出防护。

---

## 一、 核心常量

| 常量名 | 类型 | 值 | 说明 |
| :--- | :--- | :--- | :--- |
| `ISO_SECTOR_SIZE` | `usize` | `2048` | ISO 9660 (ECMA-119) 标准逻辑扇区字节数 |
| `ISO_SYSTEM_AREA_SECTORS` | `usize` | `16` | 系统保留区扇区数（前 32 KiB） |
| `ISO_PVD_OFFSET` | `usize` | `32768` | 卷描述符序列起始字节偏移（`16 * 2048`） |
| `ISO_MAGIC` | `[]const u8` | `"CD001"` | 标准卷描述符标识符 |
| `VD_TYPE_BOOT` | `u8` | `0` | 引导记录卷描述符 |
| `VD_TYPE_PRIMARY` | `u8` | `1` | 主卷描述符（PVD） |
| `VD_TYPE_SUPPLEMENTARY` | `u8` | `2` | 补充卷描述符（如 Joliet） |
| `VD_TYPE_TERMINATOR` | `u8` | `255` | 卷描述符集终止符 |
| `ISO_FLAG_HIDDEN` | `u8` | `0x01` | 目录记录隐藏标志位 |
| `ISO_FLAG_DIRECTORY` | `u8` | `0x02` | 目录记录目录标志位 |

---

## 二、 数据结构

### `IsoPrimaryVolume`
主卷描述符（PVD）的零拷贝视图：
- `system_id: []const u8` — 系统标识（已去尾空格）
- `volume_id: []const u8` — 卷标识（已去尾空格）
- `volume_space_size: u32` — 逻辑块总数（已校验 LE/BE 双端一致）
- `logical_block_size: u16` — 逻辑块字节数（已校验 LE/BE 双端一致且非 0）
- `path_table_size: u32` — 路径表字节数（已校验 LE/BE 双端一致）
- `root_extent_lba: u32` — 根目录 Extent 起始逻辑块号（LBA）
- `root_data_len: u32` — 根目录 Extent 数据字节长度
- `publisher_id: []const u8` — 发布者标识（已去尾空格）
- `application_id: []const u8` — 应用程序标识（已去尾空格）

### `IsoDirEntry`
单条 ISO 9660 目录记录的零拷贝视图：
- 字段：`extent_lba: u32`、`data_len: u32`、`flags: u8`、`name: []const u8`
- 方法：
  - `isDir(): bool` — 是否为目录项（`flags & 0x02 != 0`）
  - `isHidden(): bool` — 是否为隐藏项（`flags & 0x01 != 0`）
  - `isDot(): bool` — 是否为当前目录 `.`（`"\x00"`）
  - `isDotDot(): bool` — 是否为上级目录 `..`（`"\x01"`）
  - `baseName(): []const u8` — 剥离 `;1` 等版本号后缀后的基础文件名切片

### `IsoDirIter`
零堆分配目录记录迭代器，自动跳过扇区末尾 `record_len == 0` 填充并对齐到下一 2048B 扇区：
- `IsoDirIter.init(data: []const u8, lba: u32, data_len: u32, block_size: u16): IsoDirIter`
- `IsoDirIter.fromRoot(data: []const u8, pvd: IsoPrimaryVolume): IsoDirIter`
- `IsoDirIter.fromEntry(data: []const u8, pvd: IsoPrimaryVolume, entry: IsoDirEntry): IsoDirIter`
- `next(*_): IsoDirEntry?` — 返回下一条合法目录项；到达末尾或遇到畸形记录返回 `null`（畸形时 `had_error == true`）

---

## 三、 核心函数

- `isIso9660(data: []const u8): bool`  
  校验 `data.len >= 32768 + 2048` 且偏移 `32768 + 1..6` 为 `"CD001"`、版本号为 `1`。
- `parseIsoPvd(data: []const u8): IsoPrimaryVolume?`  
  从扇区 16 起按 2048B 步进扫描卷描述符，遇 `VD_TYPE_TERMINATOR` 停止，解析首个 `VD_TYPE_PRIMARY` 并校验所有 both-endian 字段一致性。
- `findIsoEntry(data: []const u8, pvd: IsoPrimaryVolume, name: []const u8): IsoDirEntry?`  
  在根目录中按名称查找目录项（同时匹配完整 `name` 与 `baseName()`）。
- `readIsoFileData(data: []const u8, pvd: IsoPrimaryVolume, entry: IsoDirEntry): ([]const u8)?`  
  零拷贝返回普通文件的数据切片，防乘法/加法溢出及镜像越界；若 `entry` 为目录或越界则返回 `null`。

---

## 四、 使用示例

```typescript
import {
    isIso9660,
    parseIsoPvd,
    IsoDirIter,
    findIsoEntry,
    readIsoFileData,
} from "cap:iso";

fn inspectImage(image: []const u8) {
    if (!isIso9660(image)) return;
    if (pvd := parseIsoPvd(image)) {
        let it = IsoDirIter.fromRoot(image, pvd);
        while (entry := it.next()) {
            if (!entry.isDot() && !entry.isDotDot() && !entry.isDir()) {
                if (content := readIsoFileData(image, pvd, entry)) {
                    // 零拷贝访问 content 切片与 entry.baseName()
                    * = content.len;
                }
            }
        }
        if (readme := findIsoEntry(image, pvd, "README.TXT")) {
            if (text := readIsoFileData(image, pvd, readme)) {
                * = text.len;
            }
        }
    }
}
```
