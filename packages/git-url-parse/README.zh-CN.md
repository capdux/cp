[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# git-url-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Git repository URL parser and normalizer (SCP SSH, HTTPS, SSH+port, hosted shorthands, GitLab nested groups) for CP

## 安装与导入

```bash
cap add git-url-parse
```

```cp
import { toLowerAscii } from "git-url-parse";
```

## 核心导出 API

- `export enum GitProtocol`
- `export enum GitHostKind`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn trimWs(s: string): string;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn startsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn indexOfChar(s: string, ch: u8): usize?;`
- `export fn lastIndexOfChar(s: string, ch: u8): usize?;`
- `export fn indexOfSubstr(s: string, sub: string): usize?;`
- `export fn hasWhitespace(s: string): bool;`
- `export fn parsePort(s: string): u16?;`
- `export fn detectHostKind(host: string): GitHostKind;`
- `export fn stripDotGit(s: string): string;`
- `export fn isValidSegment(s: string): bool;`
- `export fn parseGitUrl(raw: string): ParsedGitUrl;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendU16(out: []u8, pos: usize, val: u16): usize;`
- `export fn toHttpsUrl(parsed: ParsedGitUrl, out: []u8): string;`
- `export fn toSshUrl(parsed: ParsedGitUrl, out: []u8): string;`
- `export fn toBrowseUrl(parsed: ParsedGitUrl, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`git-url-parse@1.0.0`)，许可证：MIT
