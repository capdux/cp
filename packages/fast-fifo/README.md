[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# fast-fifo

Zero-heap O(1) power-of-two ring-buffer FIFO queues (`FifoI64`, `FifoStr`) in pure CP.

## API

- `newFifoI64(): FifoI64` — `push`, `shift`, `peek`, `len`, `isEmpty`, `isFull`, `clear`
- `newFifoStr(): FifoStr` — `push`, `shift`, `peek`, `len`, `isEmpty`, `clear`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`fast-fifo@1.3.2`)
- **Upstream Repository**: https://github.com/mafintosh/fast-fifo
- **License**: MIT (clean-room CP implementation)
