# @cap/tinyvec

Clean-room MIT, 100%-safe zero-heap inline stack vectors (`TinyVecI64`, `TinyVecU32`) for CP — modeled on Rust's `tinyvec` crate.

## API
- `TinyVecI64` (`new`, `len`, `capacity`, `isEmpty`, `isFull`, `get`, `first`, `last`, `push`, `pop`, `insert`, `remove`, `swapRemove`, `truncate`, `clear`, `reverse`, `dedup`, `extendFromSlice`, `binarySearch`, `sum`)
- `TinyVecU32` (`new`, `len`, `isEmpty`, `push`, `pop`, `get`, `swapRemove`, `clear`)
