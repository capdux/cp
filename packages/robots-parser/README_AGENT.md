# robots-parser@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9309 robots.txt parser and URL allow/disallow evaluator for CP. | License: MIT
> Import: `import { ... } from "robots-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn isAllowed(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool;`
- `export fn crawlDelay(robots_txt: &[]u8, user_agent: &[]u8): i64;`
- `export fn firstSitemap(robots_txt: &[]u8): &[]u8;`
