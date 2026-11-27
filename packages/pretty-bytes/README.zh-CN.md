[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pretty-bytes

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation human-readable byte and bit size formatter supporting SI/IEC, bits, signed, and compact modes (npm pretty-bytes parity)

## 安装与导入

```bash
cap add pretty-bytes
```

```cp
import { prettyBytesWith } from "pretty-bytes";
```

## 核心导出 API

- `export fn prettyBytesWith(num: f64, opts: PrettyBytesOpts, out: []u8): usize;`
- `export fn prettyBytes(num: f64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pretty-bytes@1.0.0`)，许可证：MIT
