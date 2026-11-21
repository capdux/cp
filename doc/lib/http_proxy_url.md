# `cap:http_proxy_url` — HTTP / SOCKS Proxy URL Parser & Env Selector

Zero-allocation HTTP/HTTPS/SOCKS4/SOCKS5/SOCKS5H proxy URL parser (with credentials & IPv6 support) and `HTTP_PROXY`/`HTTPS_PROXY`/`ALL_PROXY`/`NO_PROXY` selector.

## Exports

- `ProxyScheme`, `parseProxyScheme`, `defaultProxyPort`
- `ProxyUrl` — `parse(raw): ProxyUrl?`, `isSocks()`, `resolvesDnsRemotely()`
- `ProxyEnv` & `selectProxy(env, target_scheme, target_host): ProxyUrl?`
