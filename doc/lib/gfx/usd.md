# `cap:usd` — 零分配 Pixar OpenUSD (`.usdc` / `.usda` / `.usdz`) 容器解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

> **模块路径**：`lib/usd/`（入口 `lib/usd/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，支持 Pixar OpenUSD Crate (`.usdc`) 88B Bootstrap 与 TOC 区段表零拷贝解析、`.usda` 1.0 文本阶段元数据解析以及 `.usdz` 未压缩首资产探测。

---

## 一、导入方式

```cp
import {
    USDC_MAGIC,
    USDC_BOOTSTRAP_SIZE,
    USDC_SECTION_NAME_LEN,
    USDC_SECTION_ENTRY_SIZE,
    USDA_MAGIC,
    UsdFormat,
    UsdcBootstrap,
    UsdcSection,
    UsdaHeader,
    UsdcSectionIter,
    isUsdc,
    isUsda,
    isUsdz,
    detectUsdFormat,
    parseUsdcBootstrap,
    findUsdcSection,
    parseUsdaHeader,
} from "cap:gfx";
```

---

## 二、核心常量与数据结构

| 符号 | 类型 / 字段 | 说明 |
| :--- | :--- | :--- |
| `USDC_MAGIC` | `[]const u8 = "PXR-USDC"` | 8 字节 OpenUSD Crate 二进制魔数 |
| `USDC_BOOTSTRAP_SIZE` | `usize = 88` | 88B Crate Bootstrap 头固定长度 |
| `USDC_SECTION_ENTRY_SIZE` | `usize = 32` | TOC 每条区段项长度（16B NUL 填充名 + 8B start + 8B size） |
| `UsdFormat` | `usdc`, `usda`, `usdz`, `unknown` | 容器格式枚举 |
| `UsdcBootstrap` | `version: [8]u8`, `version_major/minor/patch: u8`, `toc_offset: u64`, `section_count: u64` | Crate 头部与 TOC 入口信息 |
| `UsdcSection` | `name: []const u8`, `start: u64`, `size: u64`, `payload: []const u8` | 零拷贝 Crate TOC 区段（`TOKENS`/`STRINGS`/`FIELDS`/`FIELDSETS`/`PATHS`/`SPECS` 等） |
| `UsdaHeader` | `version`, `default_prim`, `up_axis`, `meters_per_unit: f64`, `has_meters_per_unit: bool` | `.usda` 文本头与阶段级元数据 |

---

## 三、核心 API

- `detectUsdFormat(data)` / `isUsdc(data)` / `isUsda(data)` / `isUsdz(data)` — 容器类型探测（`isUsdz` 严格校验 ZIP Local Header `PK\x03\x04`、未压缩 Store `method == 0` 且首文件后缀为 `.usd`/`.usdc`/`.usda`）。
- `parseUsdcBootstrap(data: []const u8): UsdcBootstrap?` — 解析 88B Bootstrap 与 TOC 区段数，全程防整型乘法/加法溢出。
- `UsdcSectionIter.init(data, boot)` / `next(*_): UsdcSection?` — 零拷贝迭代全部 TOC 区段。
- `findUsdcSection(data, boot, name): UsdcSection?` — 按区段名查找零拷贝区段切片。
- `parseUsdaHeader(text: []const u8): UsdaHeader?` — 解析 `#usda 1.0` 及可选 `(...)` 阶段级元数据块（支持跳过嵌套 `{...}`、多行字符串与 `#` 注释）。

---

## 四、使用示例

```cp
import { parseUsdcBootstrap, findUsdcSection, parseUsdaHeader } from "cap:gfx";

fn inspectUsd(data: []const u8) {
    if (boot := parseUsdcBootstrap(data)) {
        if (tokens := findUsdcSection(data, boot, "TOKENS")) {
            * = tokens.payload;
        }
    } else if (usda := parseUsdaHeader(data)) {
        * = usda.default_prim;
    }
}
```
