[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/https-proxy-agent

Zero-heap HTTP/HTTPS proxy URL parser (IPv4, hostname, IPv6, Basic credentials) and RFC 9110 `CONNECT` tunnel handshake builder in pure CP (covers `https-proxy-agent` and `http-proxy-agent`).

## API

- `parseProxyUrl(url: string): ProxyEndpoint`
- `buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`https-proxy-agent@7.0.6`, `http-proxy-agent@7.0.2`)
- **Upstream Repository**: https://github.com/TooTallNate/proxy-agents
- **License**: MIT (clean-room CP implementation)
