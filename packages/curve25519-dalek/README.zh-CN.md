[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# curve25519-dalek

> 版本：`1.0.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation constant-time RFC 7748 X25519 elliptic curve Diffie-Hellman and GF(2^255-19) field arithmetic (curve25519-dalek parity) for CP

## 安装与导入

```bash
cap add curve25519-dalek
```

```cp
import { c25Carry } from "curve25519-dalek";
```

## 核心导出 API

- `export fn c25Carry(o: []i64);`
- `export fn c25Sel(p: []i64, q: []i64, b: i64);`
- `export fn clampScalar(k: [32]u8): [32]u8;`
- `export fn x25519(scalar: [32]u8, u_coord: [32]u8): [32]u8;`
- `export fn x25519Basepoint(scalar: [32]u8): [32]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`curve25519-dalek@1.0.0`)，许可证：BSD-3-Clause
