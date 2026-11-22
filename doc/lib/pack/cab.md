# `cap:cab` — 零分配 Microsoft Cabinet (`.cab`) 归档解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:pack`，导入请使用 `import { ... } from "cap:pack"`。

> **模块路径**：`lib/cab/`（导入方式：`import { ... } from "cap:pack";`）  
> **特性**：100% 纯 CP、零堆分配、零拷贝 `\0` 文件名切片、完整支持 `CFHEADER` 保留区与多卷前后链偏移计算。

---

## 一、 核心常量与枚举

| 常量 / 枚举 | 值 | 说明 |
| :--- | :--- | :--- |
| `CAB_MAGIC` | `"MSCF"` | 4 字节 Cabinet 文件签名 |
| `CAB_HEADER_SIZE` | `36` | 固定 `CFHEADER` 字节数 |
| `CAB_FOLDER_SIZE` | `8` | 固定 `CFFOLDER` 基础字节数（实际步长为 `8 + cb_cf_folder`） |
| `CAB_FLAG_PREV` | `0x0001` | 头后含前卷文件名与磁盘名（2 个 `\0` 字符串） |
| `CAB_FLAG_NEXT` | `0x0002` | 头后含后卷文件名与磁盘名（2 个 `\0` 字符串） |
| `CAB_FLAG_RESERVE` | `0x0004` | 头后含 4B 保留尺寸字段与 `ab_reserve` 字节区 |
| `CabCompression` | `none=0`, `mszip=1`, `quantum=2`, `lzx=3`, `unknown=255` | 按 `type_compress & 0x000F` 解码的压缩算法枚举 |

---

## 二、 数据结构与迭代器

### `CabHeader`
解析 36B `CFHEADER` 及其可选扩展区域：
- 字段：`cb_cabinet: u32`、`coff_files: u32`、`version_minor: u8`、`version_major: u8`、`c_folders: u16`、`c_files: u16`、`flags: u16`、`set_id: u16`、`i_cabinet: u16`、`cb_cf_header: u16`、`cb_cf_folder: u8`、`cb_cf_data: u8`、`folders_offset: usize`

### `CabFolder` & `CabFolderIter`
- `CabFolder`：`coff_cab_start: u32`、`c_cf_data: u16`、`type_compress: u16`、`compression: CabCompression`
- `CabFolderIter.init(data, header)` / `CabFolderIter.fromData(data)` / `next(*_): CabFolder?`（自动按 `8 + cb_cf_folder` 步进）

### `CabFile` & `CabFileIter`
- `CabFile`：`cb_file: u32`、`uoff_folder_start: u32`、`i_folder: u16`、`date: u16`、`time: u16`、`attribs: u16`、`name: []const u8`
- 方法：`isUtf8Name(): bool`、`isReadOnly(): bool`、`isHidden(): bool`
- `CabFileIter.init(data, header)` / `CabFileIter.fromData(data)` / `next(*_): CabFile?`

---

## 三、 核心函数

- `isCab(data: []const u8): bool` — 校验长度 >= 36、魔数 `"MSCF"`、`reserved1 == 0`、`version_major == 1 && version_minor == 3`
- `parseCabHeader(data: []const u8): CabHeader?` — 解析 `CFHEADER` 与可选扩展区并计算精确 `folders_offset`
- `decodeCabCompression(type_compress: u16): CabCompression` — 按低 4 位掩码解码压缩算法
- `findCabFile(data: []const u8, name: []const u8): CabFile?` — 按精确文件名查找首个匹配的 `CFFILE` 条目

---

## 四、 使用示例

```typescript
import {
    isCab,
    parseCabHeader,
    CabFolderIter,
    CabFileIter,
    findCabFile,
} from "cap:pack";

fn inspectCab(bytes: []const u8) {
    if (!isCab(bytes)) return;
    if (hdr := parseCabHeader(bytes)) {
        let fit = CabFolderIter.init(bytes, hdr);
        while (folder := fit.next()) {
            * = folder.compression;
        }
        let files = CabFileIter.init(bytes, hdr);
        while (file := files.next()) {
            * = file.name.len;
        }
        if (target := findCabFile(bytes, "setup.inf")) {
            * = target.cb_file;
        }
    }
}
```
