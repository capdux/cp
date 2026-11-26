# `@cap/sqids` — 零分配 YouTube 风格可逆无脏词短 ID 编码器 / Zero-Allocation Sqids Encoder & Decoder

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqids` ([https://github.com/sqids/sqids-javascript](https://github.com/sqids/sqids-javascript))

> 独立 Clean-Room 纯 CP 实现，严格遵循 Sqids 官方算法规范，对标 npm `sqids` / `hashids` 与 Rust `sqids` / `harsh` | 许可协议：MIT

## 安装 / Installation

```bash
cap add @cap/sqids
```

## 概述 / Overview

`@cap/sqids` 将一个或多个无符号 64 位整数（`u64`）无损编码为紧凑、URL 友好、确定性且不含常见英文不雅词的短字符串 ID，并支持 100% 可逆解码：

- **零堆分配**：所有字母表洗牌、进制编码与解码均在栈上完成，编码结果直接写入调用方缓冲区 `out: []u8`；
- **完整 Sqids 算法规范**：支持默认 62 字符字母表、字母表合法性校验（长度 `>= 3`、纯 ASCII、无重复字符）、确定性 Fisher-Yates 洗牌与旋转反转；
- **最小长度填充（`min_length`）**：支持 `0..255` 最小长度填充，自动插入边界分隔符并在解码时精准剥离填充；
- **内置脏词屏蔽（Blocklist）**：自动检测常见英文不雅词与数字变体，通过自增偏移重试生成干净短 ID；
- **100% 无损往返**：对任意 `u64`（含 `0` 与 `u64::MAX`）及多数字数组严格保证 `decode(encode(nums)) == nums`。

## 导出 API / Exported API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `DEFAULT_ALPHABET` | 默认 62 字符字母表 (`a-zA-Z0-9`) |
| `SqidsError` | `None` / `AlphabetTooShort` / `AlphabetTooLong` / `AlphabetNonAscii` / `AlphabetDuplicateChar` |
| `SqidsOptions` | `{ alphabet: string = DEFAULT_ALPHABET, min_length: u8 = 0 }` |
| `Sqids.initDefault(): Sqids` | 使用默认字母表与 `min_length = 0` 创建实例 |
| `Sqids.init(opts: SqidsOptions): Sqids` | 使用自定义字母表与最小长度创建实例并校验 |
| `sqids.isValid(): bool` | 返回配置是否合法 |
| `sqids.encode(numbers: []const u64, out: []u8): string` | 编码数字数组至 `out` |
| `sqids.encodeOne(n: u64, out: []u8): string` | 快捷编码单个 `u64` |
| `sqids.decode(id: string, out: []u64): usize` | 解码至 `out`，返回还原的数字个数（非法输入返回 `0`） |
| `sqids.decodeOne(id: string): u64?` | 快捷解码单个 `u64` |

## 使用示例 / Examples

```cp
import { Sqids, SqidsOptions } from "@cap/sqids";

fn demo() {
    const sqids = Sqids.initDefault();

    // 1. 单数字编码与解码
    let buf: [64]u8 = undefined;
    const id = sqids.encodeOne(123456, buf[0..]);
    if (decoded := sqids.decodeOne(id)) {
        // decoded == 123456
        _ = decoded;
    }

    // 2. 多数字数组编码与最小长度填充
    const custom = Sqids.init(SqidsOptions{ min_length: 8 });
    const nums: [3]u64 = [1, 2, 3];
    const multi_id = custom.encode(nums[0..], buf[0..]);
    let out_nums: [8]u64 = undefined;
    const count = custom.decode(multi_id, out_nums[0..]);
    // count == 3, out_nums[0..3] == { 1, 2, 3 }
    _ = count;
}
```
