# requires-port@1.0.0 (Agent Quickref)

> Zero-heap checker determining whether a port number is non-default for a given URL protocol | License: MIT
> Import: `import { ... } from "requires-port";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultPortForProtocol(protocol: string): u16;`
- `export fn requiresPort(port: u16, protocol: string): bool;`
