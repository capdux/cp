# hashring — AI Agent Quick Ref

Zero-allocation consistent hash ring with virtual nodes, O(log V) binary search routing, and deduplicated replica selection for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RingPoint,
    ReplicaList,
    HashRing,
    mulWrap64,
    avalanche64,
    fnv1a64Feed,
    hashKey,
    hashVnode,
} from "hashring";
```

## Core Types & Constants

```cp
class RingPoint { hash: u64, node_idx: u16 }
class ReplicaList { nodes: [8]string, count: usize } // methods: empty(...) +1
class HashRing { nodes: [32]string, node_active: [32]bool, active_nodes: usize, vnodes_per_node: u16, /* +2 fields */ } // methods: init(...) +7
```

## API Signatures

```cp
fn mulWrap64(a: u64, b: u64): u64;
fn avalanche64(x: u64): u64;
fn fnv1a64Feed(state: u64, b: u8): u64;
fn hashKey(key: string): u64;
fn hashVnode(name: string, v_idx: u16): u64;
fn eqStr(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { hashKey } from "hashring";

pub fn main() {
    let result = hashKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hashring"`; install via `cap add hashring`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
