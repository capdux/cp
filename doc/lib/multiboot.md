# `cap:multiboot` — GNU Multiboot1 & Multiboot2 引导协议零分配解析库

> **模块路径**: `lib/multiboot/`  
> **导入协议**: `import { parseMb1Header, parseMb2Header, parseMb2BootInfo, iterMb2Mmap, parseMb2Framebuffer, parseMb2Module } from "cap:multiboot";`  
> **动态挂载**: `import cap from "cap";` -> `cap.multiboot.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:multiboot` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 GNU Multiboot1 & Multiboot2 操作系统引导协议解析库：

1. **Multiboot1 & Multiboot2 内核头校验**：
   - `parseMb1Header` 验证 `0x1BADB002` 与 3 字段 32-bit 环绕和为 `0`；
   - `parseMb2Header` 验证 `0xE85250D6`、4 字段 32-bit 环绕和为 `0` 以及 `16 <= header_length <= data.len`。
2. **Multiboot2 Boot Information 8B 对齐 Tag 迭代**：
   - `parseMb2BootInfo` 校验 `total_size >= 8` 与 `reserved == 0`；
   - `Mb2TagIter` 按 8 字节边界零拷贝遍历全部 Tag 直至 `TAG_END (0)`，提供 `findTag`、`cmdline()`、`bootloaderName()` 与 `hasError()` 异常检测。
3. **强类型引导信息视图**：
   - 内置 `parseMb2BasicMemInfo`、`iterMb2Mmap`（支持 Available / Reserved / ACPI / NVS / BadRAM）、`parseMb2Framebuffer`、`parseMb2Module`。

---

## 二、 核心 API 速查表 (`lib/multiboot/multiboot.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与架构 | `MB1_HEADER_MAGIC`, `MB1_BOOTLOADER_MAGIC`, `MB2_HEADER_MAGIC`, `MB2_BOOTLOADER_MAGIC`, `MB2_ARCH_I386`, `MB2_ARCH_MIPS32` |
| 协议常量 | Tag & Mmap 类型 | `TAG_END` .. `TAG_ACPI_NEW`, `MMAP_AVAILABLE` .. `MMAP_BADRAM` |
| 内核头 | `Mb1Header` & `Mb2Header` | `parseMb1Header(data)`, `parseMb2Header(data)` |
| 引导信息 | `Mb2BootInfo` & `Mb2TagIter` | `parseMb2BootInfo(data)`, `tags()`, `findTag(t)`, `cmdline()`, `bootloaderName()`, `Mb2Tag.asString()`, `hasError()` |
| 强类型视图 | MemInfo / Mmap / FB / Module | `parseMb2BasicMemInfo(tag)`, `iterMb2Mmap(tag)`, `Mb2MmapEntry.isAvailable()`, `parseMb2Framebuffer(tag)`, `parseMb2Module(tag)` |

---

## 三、 使用示例

```typescript
import { parseMb2BootInfo, TAG_MMAP, iterMb2Mmap } from "cap:multiboot";

fn inspectBootInfo(mbi_bytes: []const u8) {
    if (info := parseMb2BootInfo(mbi_bytes)) {
        * = info.cmdline();
        if (mmap_tag := info.findTag(TAG_MMAP)) {
            if (it := iterMb2Mmap(mmap_tag)) {
                let iter = it;
                while (entry := iter.next()) {
                    if (entry.isAvailable()) {
                        * = entry.length;
                    }
                }
            }
        }
    }
}
```
