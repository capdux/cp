# `cap:pe_format` — Windows PE64 可执行文件头与节表解析/校验/构建库

> 参考来源：`/data/dev/bun/src/exe_format/pe.cp` | 吸收日期：2026-11-22

`cap:pe_format` 提供完整的 Windows PE32+ (PE64) 二进制镜像解析、结构校验、节表查询/注入、Authenticode 安全目录剥离以及标准 Windows PE 校验和计算能力，底层缓冲采用 `~init()` 确定性 RAII 生命周期管理。

---

## 快速导入

```cp
import {
    PEFile,
    DOSHeader,
    PEHeader,
    OptionalHeader64,
    SectionHeader,
    StripMode,
    StripOpts,
    Error,
    isPE,
    isPow2,
    alignUpU32,
    computePEChecksum,
    IMAGE_SCN_CNT_INITIALIZED_DATA,
    IMAGE_SCN_MEM_READ,
} from "cap:pe_format";
```

---

## API 参考

### 1. C-ABI 头部结构体与常量

| 结构体 | 物理尺寸 | 说明 |
| :--- | :---: | :--- |
| `DOSHeader` | `64 B` | MZ DOS 头（`e_magic = 0x5A4D`, `e_lfanew`） |
| `PEHeader` | `24 B` | COFF 文件头（`signature = 0x00004550`, `machine`, `number_of_sections`） |
| `DataDirectory` | `8 B` | 16 项数据目录条目（`virtual_address`, `size`） |
| `OptionalHeader64` | `240 B` | PE32+ 可选头（`magic = 0x020B`, `section_alignment`, `file_alignment`, `checksum`, `size_of_image`） |
| `SectionHeader` | `40 B` | 节表项（8 字节节名、RVA、Raw 偏移、标志位） |

### 2. 工具函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isPow2` | `pub fn isPow2(x: u32): bool` | 判断是否为 2 的幂次 |
| `alignUpU32` | `pub fn alignUpU32(v: u32, a: u32): u32!` | 按 2 的幂次向上对齐（带 `BadAlignment` / `Overflow` 防护） |
| `alignUpUsize` | `pub fn alignUpUsize(v: usize, a: usize): usize!` | `usize` 向上对齐 |
| `encodeSectionName` | `pub fn encodeSectionName(name: string): [8]u8!` | 编码 1..8 字节节名（零填充） |
| `isPE` | `pub fn isPE(data: []const u8): bool` | 快速 DOS + PE 签名魔数识别 |
| `computePEChecksum` | `pub fn computePEChecksum(raw_bytes: []const u8, checksum_off: usize): u32` | 计算标准 Windows PE 镜像校验和 |

### 3. `PEFile`（带 `~init()` RAII 自动释放）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` | `static fn parse(allocator, pe_data: []const u8): PEFile!` | 解析并验证 PE64 镜像 |
| `buildMinimalPE64` | `static fn buildMinimalPE64(allocator, code: []const u8): PEFile!` | 构建合法的最小 PE64 镜像（含 `.text` 节与校验和） |
| `bytes` / `sectionCount` | `fn bytes(): []const u8` / `fn sectionCount(): u16` | 获取当前完整镜像字节切片与节数量 |
| `getDosHeader` / `getPEHeader` / `getOptionalHeader` | `fn get*(): Header!` | 对齐安全读取各标准头部 |
| `getSectionHeader` | `fn getSectionHeader(idx: usize): SectionHeader!` | 按下标读取节表项 |
| `findSection` | `fn findSection(name: string): SectionHeader?` | 按名称查找节表项 |
| `getSectionRawData` | `fn getSectionRawData(name: string): ([]const u8)!` | 读取指定节的原始数据切片 |
| `addSection` | `fn addSection(*_, name, payload, characteristics, strip)!` | 追加新节、更新 `SizeOfImage` 并重算校验和 |
| `stripAuthenticode` | `fn stripAuthenticode(*_, opts: StripOpts)!` | 剥离尾部 Authenticode 安全目录与强制完整性标志 |
| `recomputeChecksum` | `fn recomputeChecksum(*_)!` | 重算并回写 `OptionalHeader64.checksum` |
| `validate` | `fn validate()!` | 全量校验签名、对齐、节重叠与 `SizeOfImage` 一致性 |

---

## 使用示例

```cp
import {
    PEFile,
    StripMode,
    IMAGE_SCN_CNT_INITIALIZED_DATA,
    IMAGE_SCN_MEM_READ,
} from "cap:pe_format";

fn embedPayload(allocator: std.mem.Allocator)! {
    const code: [1]u8 = [0xC3];
    let pe = PEFile.buildMinimalPE64(allocator, code[0..1])?;
    pe.addSection(".cap", "hello-pe64", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, .none)?;
    pe.validate()?;

    const raw = pe.getSectionRawData(".cap")?;
    * = raw;
} // 离开作用域自动触发 pe.~init() 释放内存
```

---

## 上游 Bun PE64 行为契约与安全边界注意事项

1. **单文件可执行体节注入与 Authenticode 剥离（`addSection` / `StripMode`）**：
   - 源自 Bun `bun build --compile` Windows 单文件打包管线：向已签名 PE64 镜像注入自定义数据节（如 `.cap`）前，需按 `StripMode` 清理既有 Authenticode 安全目录并重新计算标准校验和（`computePEChecksum`），避免 Windows 加载器因签名/校验和失效拒绝执行。
2. **文件对齐与虚拟地址对齐校验（`validate`）**：
   - `FileAlignment` 与 `SectionAlignment` 必须为 2 的幂（`isPow2`），`addSection` 自动按 `alignUpU32` 对齐原始偏移与虚拟大小，防止镜像加载偏移错位。
3. **RAII 确定性释放（`~init()`）**：
   - `PEFile` 持有内部镜像缓冲区，离开作用域自动触发 `~init()` 释放内存，无需手动调用释放函数。
