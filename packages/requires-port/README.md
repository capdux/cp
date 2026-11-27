[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# requires-port

Zero-heap protocol default port resolver and checker in pure CP.

## API

- `defaultPortForProtocol(protocol: string): u16`
- `requiresPort(port: u16, protocol: string): bool` — accepts `"http"`, `"http:"`, or `"http://"` and returns `false` for default ports (`http:80`, `https:443`, `ws:80`, `wss:443`, `ftp:21`, `gopher:70`, `file:*`).

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`requires-port@1.0.0`)
- **Upstream Repository**: https://github.com/unshiftio/requires-port
- **License**: MIT (clean-room CP implementation)
