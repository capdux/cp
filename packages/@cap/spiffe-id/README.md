# @cap/spiffe-id

Zero-allocation CNCF SPIFFE ID (`spiffe://<trust-domain>/<workload-path>`) parser, strict security validator, and formatter for CP.

## Features

- **Zero heap allocation**: zero-copy `SpiffeId` slicing and caller-supplied output buffers.
- **Strict CNCF SPIFFE compliance**: enforces lowercase `spiffe://` scheme, max 2048-byte total length, lowercase trust domain (`1..255`), and rejects `.` / `..` path traversal, empty `//` segments, trailing slashes, ports, userinfo, queries, and fragments.
- **Membership & formatting**: `belongsToTrustDomain`, `isValidTrustDomain`, `isValidPath`, and `formatSpiffeId`.

## Usage

```cp
import { parseSpiffeId, belongsToTrustDomain, formatSpiffeId } from "@cap/spiffe-id";

const id = parseSpiffeId("spiffe://prod.example.org/ns/default/sa/api-server");
const ok = belongsToTrustDomain(id, "prod.example.org"); // true
```
