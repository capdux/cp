[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# crc8

Zero-allocation CRC-8 checksum library (SMBus, Maxim 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc8` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `crc8` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add crc8
```

```cp
import {
    Crc8Hasher,
    reflect8,
    crc8Smbus,
    crc8Maxim,
    crc8SaeJ1850,
    crc8Autosar,
    crc8Cdma2000,
    crc8Bluetooth,
} from "crc8";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Crc8Hasher` | `class` | 字段: `table: [256]u8, state: u8, init_val: u8, ref_in: bool, ref_out: bool, xor_out: u8` · 方法: `initCustom, initSmbus, initMaxim, initSaeJ1850, initAutosar, initCdma2000, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `reflect8` | `(b: u8): u8` | 调用 `reflect8` 并返回 `u8`。 |
| `crc8Smbus` | `(data: &[]u8): u8` | 调用 `crc8Smbus` 并返回 `u8`。 |
| `crc8Maxim` | `(data: &[]u8): u8` | 调用 `crc8Maxim` 并返回 `u8`。 |
| `crc8SaeJ1850` | `(data: &[]u8): u8` | 调用 `crc8SaeJ1850` 并返回 `u8`。 |
| `crc8Autosar` | `(data: &[]u8): u8` | 调用 `crc8Autosar` 并返回 `u8`。 |
| `crc8Cdma2000` | `(data: &[]u8): u8` | 调用 `crc8Cdma2000` 并返回 `u8`。 |
| `crc8Bluetooth` | `(data: &[]u8): u8` | 调用 `crc8Bluetooth` 并返回 `u8`。 |

## 最小调用示例

```cp
import { reflect8 } from "crc8";

pub fn main() {
    let result = reflect8(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
