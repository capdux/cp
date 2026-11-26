// Type definitions for CP

export fn memchr(needle: u8, haystack: []u8): usize?;
export fn memchr2(n1: u8, n2: u8, haystack: []u8): usize?;
export fn memchr3(n1: u8, n2: u8, n3: u8, haystack: []u8): usize?;
export fn memrchr(needle: u8, haystack: []u8): usize?;
export fn memrchr2(n1: u8, n2: u8, haystack: []u8): usize?;
export fn memrchr3(n1: u8, n2: u8, n3: u8, haystack: []u8): usize?;
export fn countByte(needle: u8, haystack: []u8): usize;
export fn memmem(haystack: []u8, needle: []u8): usize?;
export fn memrmem(haystack: []u8, needle: []u8): usize?;
export fn memmemStr(haystack: string, needle: string): usize?;
