# `cap:cuid2` — 纯 CP 抗碰撞 CUID2 Base36 唯一 ID 生成引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 概述

`cap:cuid2`（亦支持 `cap.cuid2` 动态挂载）实现 CUID2 规范的抗碰撞安全 ID 生成器：融合高分辨率时间戳、原子单调计数器、CSPRNG 随机盐与节点指纹，通过 SHA3-512 混合并编码为 `[a-z][0-9a-z]*` 的 Base36 紧凑字符串（默认 24 字符）。

## 核心类型与 API

- `Cuid2Error`: `InvalidLength`、`InvalidCharacter`。
- `Cuid2Generator`:
  - `init(seed_entropy) -> Cuid2Generator`
  - `nextInto(&self, out) -> Cuid2Error![]const u8`
  - `next24(&self) -> [24]u8`
- `isValidCuid2(s) -> bool`

## 示例

```cp
import { Cuid2Generator, isValidCuid2 } from "cap:codec";

let gen = Cuid2Generator.init("worker-node-1");
const id = gen.next24();
```
