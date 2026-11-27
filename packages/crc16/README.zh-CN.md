[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# crc16

Zero-heap industrial CRC-16 checksums (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, USB) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc16` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `crc16` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add crc16
```

```cp
import {
    Crc16Hasher,
    reflectU16,
    crc16BuildTable,
    crc16CcittFalse,
    crc16Xmodem,
    crc16Modbus,
    crc16Kermit,
    crc16Arc,
} from "crc16";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Crc16Hasher` | `class` | 字段: `table: [256]u16, poly: u16, init_val: u16, xor_out: u16, ref_in: bool, ref_out: bool, +1 项` · 方法: `initCustom, initCcittFalse, initXmodem, initModbus, initKermit, initArc, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `reflectU16` | `(v: u16): u16` | 调用 `reflectU16` 并返回 `u16`。 |
| `crc16BuildTable` | `(poly: u16, ref_in: bool): [256]u16` | 调用 `crc16BuildTable` 并返回 `[256]u16`。 |
| `crc16CcittFalse` | `(data: &[]u8): u16` | 调用 `crc16CcittFalse` 并返回 `u16`。 |
| `crc16Xmodem` | `(data: &[]u8): u16` | 调用 `crc16Xmodem` 并返回 `u16`。 |
| `crc16Modbus` | `(data: &[]u8): u16` | 调用 `crc16Modbus` 并返回 `u16`。 |
| `crc16Kermit` | `(data: &[]u8): u16` | 调用 `crc16Kermit` 并返回 `u16`。 |
| `crc16Arc` | `(data: &[]u8): u16` | 调用 `crc16Arc` 并返回 `u16`。 |
| `crc16Usb` | `(data: &[]u8): u16` | 调用 `crc16Usb` 并返回 `u16`。 |

## 最小调用示例

```cp
import { reflectU16 } from "crc16";

pub fn main() {
    let result = reflectU16(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
