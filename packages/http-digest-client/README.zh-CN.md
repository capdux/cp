[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http-digest-client

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP

## 安装与导入

```bash
cap add http-digest-client
```

```cp
import { digIsSpace } from "http-digest-client";
```

## 核心导出 API

- `export fn digIsSpace(b: u8): bool;`
- `export fn trimSlice(s: string): string;`
- `export fn stripDigestPrefix(header: string): string;`
- `export fn unquoteVal(v: string): string;`
- `export fn parseChallenge(header: string): DigestChallenge;`
- `export fn parseAuthorization(header: string): DigestResponse;`
- `export fn supportsQop(qop_list: string, target: string): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushQuotedField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;`
- `export fn pushTokenField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;`
- `export fn formatAuthorization(resp: DigestResponse, out: []u8): usize;`
- `export fn formatChallenge(ch: DigestChallenge, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`http-digest-client@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`digest_auth@1.0.0`)，许可证：MIT
