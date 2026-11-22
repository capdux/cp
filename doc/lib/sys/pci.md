# `cap:pci` — PCI / PCIe 配置空间、BAR 与 Capability 链表零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配（zero-alloc）的 PCI（256B）与 PCIe（4096B）配置空间头、32/64-bit Memory & I/O BAR 解码以及标准/扩展 Capability 链表解析库。

## 导入

```cp
import {
    PCI_CONFIG_SIZE,
    PCIE_EXT_CONFIG_SIZE,
    PCI_HEADER_TYPE_ENDPOINT,
    PCI_HEADER_TYPE_BRIDGE,
    PCI_HEADER_MULTIFUNC_BIT,
    PCI_STATUS_CAP_LIST,
    CAP_PM,
    CAP_MSI,
    CAP_VENDOR,
    CAP_PCIE,
    CAP_MSIX,
    EXT_CAP_AER,
    EXT_CAP_ACS,
    EXT_CAP_SRIOV,
    EXT_CAP_RESIZABLE_BAR,
    CLASS_MASS_STORAGE,
    CLASS_NETWORK,
    CLASS_DISPLAY,
    CLASS_BRIDGE,
    CLASS_SERIAL_BUS,
    PciHeader,
    parsePciHeader,
    PciBar,
    decodePciBar,
    PciCap,
    PciCapIter,
    iterPciCaps,
    findPciCap,
    PcieExtCap,
    PcieExtCapIter,
    iterPcieExtCaps,
    findPcieExtCap,
} from "cap:sys";
```

## 核心 API

- **配置头解析**：`parsePciHeader(cfg): PciHeader?`（`headerType()`, `isMultiFunction()`, `hasCapabilities()`, `isEndpoint()`, `isBridge()`, `isPresent()`）。
- **BAR 解码**：`decodePciBar(cfg, bar_idx): PciBar?`（支持 32-bit Mem、64-bit Prefetchable Mem 合并相邻高低 32 位及 I/O Space BAR）。
- **Standard Capability 链表**：`iterPciCaps(cfg)`、`findPciCap(cfg, cap_id)`（`cap_ptr & 0xFC`，<= 48 跳防环校验）。
- **PCIe Extended Capability 链表**：`iterPcieExtCaps(cfg)`、`findPcieExtCap(cfg, ext_cap_id)`（从 `0x100` 起遍历 4KB 空间，<= 96 跳防环校验）。

## 示例

```cp
import { CAP_PCIE, EXT_CAP_SRIOV, parsePciHeader, decodePciBar, findPciCap, findPcieExtCap } from "cap:sys";

fn example(cfg: []const u8) {
    if (hdr := parsePciHeader(cfg)) {
        * = hdr.vendor_id;
        * = decodePciBar(cfg, 0);
        * = findPciCap(cfg, CAP_PCIE);
        * = findPcieExtCap(cfg, EXT_CAP_SRIOV);
    }
}
```
