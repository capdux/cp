# `cap:dicom` — 零分配医学影像 DICOM PS3.10 (`.dcm`) 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

> **模块路径**：`lib/dicom/`（入口 `lib/dicom/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 DICOM PS3.10 / PS3.5 规范：校验 128 字节前导与 `"DICM"` 魔数，Group `0x0002`（File Meta Information）强制按 Explicit VR Little Endian 解析，读取 `(0x0002, 0x0010)` Transfer Syntax UID 后自动切换后续数据集为 Explicit VR LE / Implicit VR LE / Explicit VR BE 模式，完整支持长/短 Explicit VR 编码与 `0xFFFFFFFF` 未定义长度序列（`SQ` / 封装 `PixelData`）安全定界跳过。

---

## 一、导入方式

```cp
import {
    DICOM_PREAMBLE_SIZE,
    DICOM_MAGIC,
    DICOM_DATA_OFFSET,
    DICOM_UNDEFINED_LEN,
    TS_IMPLICIT_VR_LE,
    TS_EXPLICIT_VR_LE,
    TS_EXPLICIT_VR_BE,
    DicomTag,
    TAG_TRANSFER_SYNTAX,
    TAG_PATIENT_NAME,
    TAG_PATIENT_ID,
    TAG_MODALITY,
    TAG_ROWS,
    TAG_COLUMNS,
    TAG_BITS_ALLOCATED,
    TAG_PIXEL_DATA,
    DicomElement,
    DicomIter,
    DicomSummary,
    isDicom,
    parseDicomSummary,
} from "cap:image";
```

---

## 二、核心常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `DICOM_PREAMBLE_SIZE` | `128` | PS3.10 文件前导字节长度 |
| `DICOM_MAGIC` | `"DICM"` | 偏移 `128..132` 处 4 字节魔数 |
| `DICOM_DATA_OFFSET` | `132` | 首个 Data Element 起始偏移 |
| `DICOM_UNDEFINED_LEN` | `0xFFFFFFFF` | 未定义长度标记（`SQ` / 封装 `PixelData`） |
| `TS_IMPLICIT_VR_LE` | `"1.2.840.10008.1.2"` | Implicit VR Little Endian |
| `TS_EXPLICIT_VR_LE` | `"1.2.840.10008.1.2.1"` | Explicit VR Little Endian |
| `TS_EXPLICIT_VR_BE` | `"1.2.840.10008.1.2.2"` | Explicit VR Big Endian |
| `TAG_TRANSFER_SYNTAX` | `(0x0002, 0x0010)` | Transfer Syntax UID |
| `TAG_PATIENT_NAME` | `(0x0010, 0x0010)` | 患者姓名 |
| `TAG_PATIENT_ID` | `(0x0010, 0x0020)` | 患者 ID |
| `TAG_MODALITY` | `(0x0008, 0x0060)` | 检查模态（如 `CT` / `MR`） |
| `TAG_ROWS` / `TAG_COLUMNS` | `(0x0028, 0x0010)` / `0x0011` | 图像行/列分辨率 |
| `TAG_BITS_ALLOCATED` | `(0x0028, 0x0100)` | 每像素分配位数 |
| `TAG_PIXEL_DATA` | `(0x7FE0, 0x0010)` | 像素数据标签 |

---

## 三、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `isDicom(data)` | `(data: []const u8): bool` | 校验 `data.len >= 132` 且 `data[128..132] == "DICM"` |
| `DicomElement` | `group: u16`, `element: u16`, `vr: [2]u8`, `value: []const u8`, `is_undefined_len: bool` | 零拷贝数据元素；支持 `isTag(g, e)`、`matchesTag(tag)`、`asTrimmedString()`、`asU16(is_be)`、`asU32(is_be)` |
| `DicomIter` | `DicomIter.init(data): DicomIter?`; `next(*_): DicomElement?`, `isBigEndian()`, `isExplicitVr()`, `hasError()` | 零分配流式解析器，自动识别 Group `0x0002` 与 Transfer Syntax 切换 |
| `DicomSummary` | `transfer_syntax`, `modality`, `patient_name`, `patient_id`, `rows`, `columns`, `bits_allocated`, `has_pixel_data`, `element_count` | 单遍提取的影像元数据摘要 |
| `parseDicomSummary(data)` | `(data: []const u8): DicomSummary?` | 单遍零分配提取 `DicomSummary`，魔数非法或元素截断时返回 `null` |

---

## 四、使用示例

```cp
import { DicomIter, parseDicomSummary, TAG_MODALITY } from "cap:image";

fn inspectDicom(data: []const u8) {
    if (summary := parseDicomSummary(data)) {
        * = summary.modality;
        * = summary.rows;
        * = summary.columns;
    }

    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        while (elem := it.next()) {
            if (elem.matchesTag(TAG_MODALITY)) {
                * = elem.asTrimmedString();
            }
        }
    }
}
```
