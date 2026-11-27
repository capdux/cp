# mongodb-connection-string-url@1.0.0 (Agent Quickref)

> Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser, option extractor, and password-redacting formatter for CP. | License: MIT
> Import: `import { ... } from "mongodb-connection-string-url";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseMongoUri(uri: string): MongoUri;`
- `export fn getQueryOption(uri: string, key: string): string;`
- `export fn formatMongoUri(cfg: MongoUri, redact_password: bool, out: []u8): usize;`
