# @cap/ringbuf

Clean-room MIT, zero-heap power-of-two O(1) circular ring buffers (`RingBufU8`, `RingBufI64`) for CP — modeled on Rust's `ringbuf` crate.

## API
- `RingBufU8` (`new`, `len`, `capacity`, `freeLen`, `isEmpty`, `isFull`, `push`, `pushOverwrite`, `pop`, `peek`, `pushSlice`, `popSlice`, `clear`)
- `RingBufI64` (`new`, `len`, `capacity`, `isEmpty`, `isFull`, `push`, `pushOverwrite`, `pop`, `peek`, `clear`)
