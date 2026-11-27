[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# camino

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap strictly validated UTF-8 path engine Utf8PathBuf (crates.io camino equivalent)

## 安装与导入

```bash
cap add camino
```

```cp
import { camTrimTrailingSlashes } from "camino";
```

## 核心导出 API

- `export fn camTrimTrailingSlashes(data: &[]u8, len: usize): usize;`
- `export fn camSliceEq(a: &[]u8, a_start: usize, a_end: usize, b: &[]u8, b_start: usize, b_end: usize): bool;`
- `export fn camIsDot(data: &[]u8, start: usize, end: usize): bool;`
- `export fn camIsDotDot(data: &[]u8, start: usize, end: usize): bool;`
- `export fn camFileNameSpan(data: &[]u8, len: usize): CamSpan;`
- `export fn camFindLastDot(data: &[]u8, start: usize, end: usize): usize?;`
- `export fn camCopyToDst(data: &[]u8, start: usize, end: usize, dst: []u8): usize?;`
- `export fn camNextComponent(data: &[]u8, len: usize, cursor: usize): CamCompStep;`
- `export fn camParentLength(data: &[]u8, len: usize): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`camino@1.0.0`)，许可证：MIT OR Apache-2.0
