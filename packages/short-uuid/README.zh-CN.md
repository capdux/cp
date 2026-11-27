[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# short-uuid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 128-bit UUID to 22-char Flickr Base58 / Base62 / Cookie Base90 lossless short ID translator for CP

## 安装与导入

```bash
cap add short-uuid
```

```cp
import { parseHexNibble } from "short-uuid";
```

## 核心导出 API

- `export fn parseHexNibble(c: u8): u8?;`
- `export fn hexNibbleLower(v: u8): u8;`
- `export fn uuidToBytes(uuid: string, out16: []u8): bool;`
- `export fn bytesToUuid(bytes16: []u8, out36: []u8): string;`
- `export fn validateAlphabet(alphabet: string): bool;`
- `export fn findInAlphabet(alphabet: string, c: u8): u32?;`
- `export fn maxEncodedLen(base: u32): usize;`
- `export fn shortenUuidWith(uuid: string, alphabet: string, out: []u8): string?;`
- `export fn expandUuidWith(short_id: string, alphabet: string, out36: []u8): string?;`
- `export fn shortenUuid(uuid: string, out: []u8): string?;`
- `export fn expandUuid(short_id: string, out36: []u8): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`short-uuid@1.0.0`)，许可证：MIT
