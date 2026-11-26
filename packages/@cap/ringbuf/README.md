# @cap/ringbuf

Zero-allocation power-of-two masked ring deque (`RingDequeI64`) and streaming byte ring buffer (`ByteRingBuf`) for CP — clean-room MIT parity with Rust `ringbuf` and `arraydeque`.

零堆分配的 CP 高性能环形缓冲区与双端队列，采用 2 的幂容量位掩码实现 O(1) 零除法环绕，适用于音频/网络流缓冲、滑窗指标、日志队列与 Actor 信箱（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/ringbuf
```

## API Reference / 接口一览

| Symbol | Methods | Description |
| :--- | :--- | :--- |
| `RingDequeI64` | `init`, `len`, `capacity`, `isEmpty`, `isFull`, `clear`, `pushBack`, `pushFront`, `pushBackOverwrite`, `popFront`, `popBack`, `front`, `back`, `get` | 容量 64 的 O(1) 双端环形队列（支持满时滑窗覆盖并返回被挤出值） |
| `ByteRingBuf` | `init`, `len`, `freeSpace`, `isEmpty`, `isFull`, `clear`, `writeByte`, `readByte`, `peekByte`, `writeBytes`, `readBytes` | 容量 512 字节的流式环形字节缓冲区（自动跨尾回绕读写） |

## Usage / 使用示例

```cp
import { RingDequeI64, ByteRingBuf } from "@cap/ringbuf";

let dq = RingDequeI64.init();
dq.pushBack(10);
dq.pushFront(5);
const first = dq.popFront().?; // 5

let rb = ByteRingBuf.init();
rb.writeBytes("hello stream");
let out: [32]u8 = undefined;
const chunk = rb.readBytes(out[0..32]); // "hello stream"
```

## License

MIT
