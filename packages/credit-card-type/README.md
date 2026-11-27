[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# credit-card-type

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:credit-card-type` ([https://github.com/braintree/credit-card-type](https://github.com/braintree/credit-card-type))（原库名 `credit-card-type` → CP 包名 `credit-card-type`）

Zero-allocation payment card BIN network detector, Luhn validator, formatter, and PCI-DSS masker for CP — clean-room MIT parity with npm `credit-card-type` (4M/wk) and `card-validator` (3M/wk).

纯 CP 零堆分配支付卡 BIN 组织识别、Luhn 模 10 校验、标准分组格式化与 PCI-DSS 脱敏引擎。

## Install / 安装

```bash
cap add credit-card-type
```

## Supported Card Networks / 支持的卡组织

| Network | BIN Ranges | Valid Lengths | CVV / CID | Formatting |
| --- | --- | --- | --- | --- |
| `Visa` | `4` | 13, 16, 19 | 3 | `4-4-4-4` |
| `Mastercard` | `51-55`, `2221-2720` | 16 | 3 | `4-4-4-4` |
| `Amex` | `34`, `37` | 15 | 4 | `4-6-5` |
| `UnionPay` | `62` | 16-19 | 3 | `4-4-4-4` |
| `Jcb` | `3528-3589` | 16-19 | 3 | `4-4-4-4` |
| `Discover` | `6011`, `644-649`, `65` | 16-19 | 3 | `4-4-4-4` |
| `DinersClub` | `300-305`, `36`, `38-39` | 14, 16 | 3 | `4-4-4-4` |
| `Maestro` | `50`, `56-58`, `6304`, `67` | 12-19 | 3 | `4-4-4-4` |

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CardNetwork` | `enum { Visa, Mastercard, Amex, UnionPay, Jcb, Discover, DinersClub, Maestro, Unknown }` | Payment card network enum |
| `CardInfo` | `struct { network, digit_count, valid_length, luhn_ok, is_valid, cvv_len }` | Full inspection result |
| `luhnValid` | `(raw: string) -> bool` | Strip spaces/hyphens and verify Luhn mod-10 checksum |
| `inspectCard` | `(raw: string) -> CardInfo` | Detect BIN network, validate length & Luhn, and return CVV length |
| `networkName` | `(n: CardNetwork) -> string` | Human-readable display name of card network |
| `formatCardNumber` | `(raw: string, out: []u8) -> string` | Format digits (`4-6-5` for Amex, `4-4-4-4` for others) |
| `maskCardPci` | `(raw: string, out: []u8) -> string` | PCI-DSS compliant masking (first 6 + last 4, or last 4 for short PANs) |

## Usage / 使用示例

```cp
import {
    CardNetwork,
    inspectCard,
    networkName,
    formatCardNumber,
    maskCardPci,
} from "credit-card-type";

const info = inspectCard("3782-822463-10005");
// info.network == CardNetwork.Amex, info.is_valid == true, info.cvv_len == 4

let fmtBuf: [32]u8 = undefined;
const formatted = formatCardNumber("378282246310005", fmtBuf[0..32]);
// "3782 822463 10005"

let maskBuf: [32]u8 = undefined;
const masked = maskCardPci("4111-1111-1111-1111", maskBuf[0..32]);
// "411111******1111"
```
