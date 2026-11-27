[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `luhn`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:luhn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap industrial check digit validation & generation in pure CP: ISO/IEC 7812 Luhn Mod-10, Damm (2004) anti-symmetric quasigroup, and Verhoeff (1969) dihedral group D5.

零堆分配纯 CP 工业级校验码三合一引擎：ISO/IEC 7812 Luhn Mod-10、Damm 全反对称拟群算法与 Verhoeff 二面体群算法，适用于银行卡号、IMEI、税务/订单编号与防单错/相邻换位校验。

## Installation

```bash
cap add luhn
```

## API

| Function | Description |
| --- | --- |
| `luhnValidate(input)` / `luhnCheckDigit(payload)` | ISO/IEC 7812 Luhn Mod-10 validation & check digit generation |
| `dammValidate(input)` / `dammCheckDigit(payload)` | Damm algorithm (catches 100% of single-digit & adjacent transpositions including `09 <-> 90`) |
| `verhoeffValidate(input)` / `verhoeffCheckDigit(payload)` | Verhoeff dihedral group D5 check digit validation & generation |

## License

MIT
