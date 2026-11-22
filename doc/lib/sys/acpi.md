# `cap:acpi` — ACPI 6.5 RSDP / SDT / RSDT / XSDT / MADT / MCFG 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配的 ACPI 6.5 固件系统描述表解析库，支持 RSDP v1/v2 双校验和验证、通用 36B SDT 报头与整表校验、RSDT/XSDT 指针提取、MADT (`"APIC"`) 中断控制器多条目迭代以及 MCFG (`"MCFG"`) PCIe ECAM 16B 区域迭代。

## 导入

```cp
import {
    RSDP_SIGNATURE,
    RSDP_V1_SIZE,
    RSDP_V2_SIZE,
    SDT_HEADER_SIZE,
    SIG_RSDT,
    SIG_XSDT,
    SIG_FACP,
    SIG_APIC,
    SIG_MCFG,
    SIG_HPET,
    SIG_DSDT,
    MADT_LOCAL_APIC,
    MADT_IO_APIC,
    MADT_INT_SRC_OVERRIDE,
    MADT_NMI_SOURCE,
    MADT_LOCAL_APIC_NMI,
    MADT_LOCAL_APIC_ADDR_OVERRIDE,
    MADT_LOCAL_X2APIC,
    acpiChecksum,
    verifyAcpiChecksum,
    AcpiRsdp,
    parseAcpiRsdp,
    AcpiSdtHeader,
    AcpiTable,
    parseAcpiTable,
    rsdtEntryCount,
    rsdtEntryAt,
    xsdtEntryCount,
    xsdtEntryAt,
    MadtEntry,
    MadtEntryIter,
    MadtTable,
    parseMadt,
    MadtLocalApic,
    parseMadtLocalApic,
    MadtIoApic,
    parseMadtIoApic,
    MadtIntOverride,
    parseMadtIntOverride,
    McfgEntry,
    McfgEntryIter,
    parseMcfgEntries,
} from "cap:sys";
```

## 核心 API

- **校验和**：`acpiChecksum(data): u8`、`verifyAcpiChecksum(data): bool`。
- **RSDP 与 SDT 通用表**：`parseAcpiRsdp(data): AcpiRsdp?`、`parseAcpiTable(data): AcpiTable?`。
- **RSDT / XSDT**：`rsdtEntryCount(tbl)`、`rsdtEntryAt(tbl, idx): u32?`、`xsdtEntryCount(tbl)`、`xsdtEntryAt(tbl, idx): u64?`。
- **MADT (`"APIC"`)**：`parseMadt(tbl): MadtTable?`（附 `hasDual8259()`, `entries(): MadtEntryIter`），以及 `parseMadtLocalApic`、`parseMadtIoApic`、`parseMadtIntOverride`。
- **MCFG (`"MCFG"`)**：`parseMcfgEntries(tbl): McfgEntryIter?`（迭代 `McfgEntry { base_addr, segment_group, start_bus, end_bus }`）。

## 示例

```cp
import { parseAcpiTable, parseMadt, parseMadtLocalApic } from "cap:sys";

fn example(raw_table: []const u8) {
    if (tbl := parseAcpiTable(raw_table)) {
        if (madt := parseMadt(tbl)) {
            let it = madt.entries();
            while (true) {
                if (e := it.next()) {
                    if (lapic := parseMadtLocalApic(e)) {
                        * = lapic.apic_id;
                    }
                } else {
                    break;
                }
            }
        }
    }
}
```
