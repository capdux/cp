[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# env-paths

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation cross-platform application directory resolver (Linux XDG, macOS Library, Windows AppData) in pure CP

## 安装与导入

```bash
cap add env-paths
```

```cp
import { evpTrimTrailingSep } from "env-paths";
```

## 核心导出 API

- `export enum EnvPlatform`
- `export fn evpTrimTrailingSep(s: string, sep: u8): string;`
- `export fn evpAppend(out: []u8, pos: usize, s: string): usize;`
- `export fn evpAppendByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn evpAppendAppName(out: []u8, pos: usize, name: string, suffix: string): usize;`
- `export fn evpJoinUnix2(out: []u8, base: string, name: string, suffix: string): usize;`
- `export fn evpJoinUnix3(out: []u8, base: string, mid: string, name: string, suffix: string): usize;`
- `export fn evpWinAppBase(out: []u8, explicit: string, home: string, fallback_sub: string): usize;`
- `export fn evpWinDir(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize;`
- `export fn envDataDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envConfigDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envCacheDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envLogDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envTempDir(name: string, inp: EnvPathsInput, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`env-paths@1.0.0`)，许可证：MIT
