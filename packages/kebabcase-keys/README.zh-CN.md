[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kebabcase-keys

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap object key converter to kebab-case with exclude list, batch conversion, and nested dot-path support, composing decamelize-keys and map-obj (clean-room port of npm kebabcase-keys)

## 安装与导入

```bash
cap add kebabcase-keys
```

```cp
import { kckCopySlice } from "kebabcase-keys";
```

## 核心导出 API

- `export fn kckCopySlice(src_buf: [1024]u8, off: usize, len: usize, out: []u8, dst_off: usize): usize;`
- `export fn kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult;`
- `export fn kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult;`
- `export fn kebabcaseKeyInto(key: string, out: []u8): usize;`
- `export fn kebabcasePathInto(dot_path: string, out: []u8): usize;`
- `export fn kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`kebabcase-keys@1.0.0`)，许可证：MIT
