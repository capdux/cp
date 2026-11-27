# querystring@1.0.0 (Agent Quickref)

> Zero-allocation URL query string parser, stringifier & parameter extractor (Node.js querystring & npm query-string v9 parity) composing percent-encoding for CP | License: MIT
> Import: `import { ... } from "querystring";` | Full types: `src/index.d.cp`
> Deps: `percent-encoding`

## Functions
- `export fn escapeQuery(raw: string, out: []u8): usize;`
- `export fn unescapeQuery(raw: string, out: []u8): usize;`
- `export fn extractUrlQuery(url: string): string;`
- `export fn parseQueryString(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize;`
- `export fn stringifyQueryString(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize;`
- `export fn pickQueryParam(url_or_query: string, target_key: string, out: []u8): usize?;`
