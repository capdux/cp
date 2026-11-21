# `cap:http_sec_fetch` — W3C Fetch Metadata (`Sec-Fetch-*`) Resource Isolation Policy

Zero-allocation W3C Fetch Metadata (`Sec-Fetch-Site`, `Sec-Fetch-Mode`, `Sec-Fetch-Dest`, `Sec-Fetch-User`) parser and Resource Isolation Policy evaluator.

## Exports

- `FetchSite`, `FetchMode`, `FetchDest` & `parseFetchSite`, `parseFetchMode`, `parseFetchDest`
- `FetchMetadata` — `parse(...)`, `allowByResourceIsolation(is_get_or_head): bool`, `allowSameOriginApi(allow_legacy): bool`.
