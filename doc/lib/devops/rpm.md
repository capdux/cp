# `cap:rpm` — 纯 CP Linux RPM v3/v4 软件包零分配头部与 Tag 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

## 概述

`cap:rpm` 提供纯 CP、零外部依赖、零堆分配的 Linux RPM v3/v4 软件包容器解析器，支持 96 字节大端 Lead 解析（Binary/Source 识别）、Signature 与 Main Header 16 字节区段头解析（自动处理 Signature 8 字节边界填充对齐）、16 字节 Tag 索引流式迭代、字符串/大端 `u32` 安全提取以及一站式 `parseRpmPackage` 元数据与 Payload 偏移计算。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `RPM_LEAD_MAGIC` / `RPM_HEADER_MAGIC` / `TAG_*` / `RPM_TYPE_*` | RPM 魔数、常用 Tag 编号与数据类型常量 |
| `RpmLead` / `parseRpmLead(data)` | 解析 96B Lead（含 `isBinary()` / `isSource()`） |
| `RpmHeaderSection` / `parseRpmHeaderSection(data, offset)` | 解析 Header/Signature 区段头并计算 8B 对齐下一偏移 |
| `RpmTagEntry` / `RpmTagIter` | 流式遍历 16B Tag 索引项 |
| `readRpmString(store, entry)` / `readRpmU32(store, entry)` | 从 Header 存储区零拷贝安全读取 C 字符串或大端 `u32` |
| `RpmPackageInfo` / `parseRpmPackage(data)` | 一站式解析 Lead + 8B 对齐 Signature + Main Header + `payload_offset` |
| `isRpm(data)` | 校验 96B 长度与 `0xEDABEEDB` 魔数 |

## 示例

```cp
import { parseRpmPackage } from "cap:devops";

fn inspectRpm(rpm_bytes: []const u8): bool {
    if (pkg := parseRpmPackage(rpm_bytes)) {
        return pkg.lead.isBinary() && pkg.name != null && pkg.payload_offset <= rpm_bytes.len;
    }
    return false;
}
```
