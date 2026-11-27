[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/session

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official zero-heap tamper-proof cookie session plugin for cap:web (pure-CP keyed MAC + fixed-slot KV store)

## 安装与导入

```bash
cap add @web/session
```

```cp
import { sessMonotonicNowSec } from "@web/session";
```

## 核心导出 API

- `export fn sessMonotonicNowSec(): u64;`
- `export fn sessNowSec(): u64;`
- `export fn sessFnv1a64Update(seed: u64, data: &[]u8): u64;`
- `export fn sessSplitMix64(x: u64): u64;`
- `export fn sessWriteHexU64(val: u64, out_buf: []u8, offset: usize);`
- `export fn sessComputeMacHex(secret: string, id: string, out_hex: &mut [SESS_SIG_HEX_LEN]u8);`
- `export fn sessConstTimeEql(a: string, b: string): bool;`
- `export fn sessWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn sessWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn sessVerifySignedCookie(signed_cookie: string): string?;`
- `export fn sessFindValidSlotIndex(id: string, now_sec: u64): usize?;`
- `export fn setSessionTestClock(now_sec: u64);`
- `export fn resetSessionStore();`
- `export fn sessionCreate(user_val: string, now_sec: u64, out_cookie_val: []u8): usize?;`
- `export fn sessionGet(signed_cookie: string, now_sec: u64): string?;`
- `export fn sessionDestroy(signed_cookie: string): bool;`
- `export fn formatSessionSetCookie(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): usize?;`
- `export fn sessionHook(ctx: &mut Context): bool;`
- `export fn createSessionPlugin(cfg: SessionConfig): Plugin;`
- `export fn registerSession(app: &mut App, cfg: SessionConfig): &mut App;`
