# @cap/consistent-hash

Zero-allocation consistent hash ring with virtual nodes, `O(log V)` binary-search clockwise routing, and deduplicated replica selection for CP — clean-room MIT parity with Rust `hashring` and npm `hashring`.

零堆分配的 CP 一致性哈希环引擎，支持最多 32 个物理节点、256 个有序虚拟节点槽位、FNV-1a + SplitMix 雪崩哈希、`O(log V)` 二分顺时针定位与去重多副本容灾路由，适用于分布式缓存分片、多节点 `caprun` Agent 路由与负载均衡（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/consistent-hash
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `RingPoint` / `ReplicaList` | `pub struct` | 环上虚拟节点与去重副本列表（最多 8 个副本，提供 `nodeAt(i)`） |
| `HashRing.init` | `(vnodes_per_node: u16) -> HashRing` | 初始化空哈希环并设置每节点虚拟节点数 |
| `addNode` / `removeNode` | `(*_, name: string) -> bool` | 增删物理节点及其全部虚拟节点（保持环有序） |
| `getNode` | `(key: string) -> string?` | `O(log V)` 二分顺时针定位主节点 |
| `getReplicas` | `(key: string, max_replicas: usize) -> ReplicaList` | 顺时针返回最多 `max_replicas` 个去重物理副本节点 |
| `nodeCount` / `vnodeCount` | `() -> usize` | 查询活跃物理节点数与虚拟节点总数 |

## Usage / 使用示例

```cp
import { HashRing } from "@cap/consistent-hash";

let ring = HashRing.init(8);
ring.addNode("node-a");
ring.addNode("node-b");
ring.addNode("node-c");

const owner = ring.getNode("session:42").?;
const replicas = ring.getReplicas("session:42", 2);
```

## License

MIT
