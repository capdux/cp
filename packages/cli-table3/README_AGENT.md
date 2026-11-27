# `cli-table3` — AI Agent Quick Ref

Zero-dependency CLI table renderer with 5 border styles, ANSI/CJK width calculation, truncation & column alignment (npm `cli-table3` parity).

## Import

```cp
import {
    BorderStyle,
    Align,
    displayWidth,
    truncateDisplay,
    padCell,
    Table,
} from "cli-table3";
```

## Core API Signatures

```cp
enum BorderStyle: u8 { Unicode = 0, Rounded = 1, Ascii = 2, Markdown = 3, Compact = 4 }
enum Align: u8 { Left = 0, Center = 1, Right = 2 }

fn displayWidth(s: string): usize;
fn truncateDisplay(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error;
fn padCell(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error;

// Table (requires `let tbl = Table.init(alloc); defer tbl.deinit();`)
Table.init(alloc: std.mem.Allocator): Table;
tbl.deinit(*_);
tbl.setHeaders(*_, headers: []const string): void ! std.mem.Allocator.Error;
tbl.addRow(*_, row: []const string): void ! std.mem.Allocator.Error;
tbl.setAlign(*_, col: usize, alignment: Align): void ! std.mem.Allocator.Error;
tbl.setStyle(*_, style: BorderStyle);
tbl.setMaxColWidth(*_, max_width: usize);
tbl.render(alloc: std.mem.Allocator): []u8 ! std.mem.Allocator.Error;
```

## Minimal Example

```cp
const std = @import("std");
import { Table } from "cli-table3";

fn example(alloc: std.mem.Allocator): void ! std.mem.Allocator.Error {
    let tbl = Table.init(alloc);
    defer tbl.deinit();
    tbl.setStyle(.Markdown);
    const h: [2]string = ["Name", "Score"];
    const r: [2]string = ["CP", "100"];
    tbl.setHeaders(&h)?;
    tbl.addRow(&r)?;
    const out = tbl.render(alloc)?;
    defer alloc.free(out);
}
```

## Gotchas & Borrow Rules

- Unlike zero-alloc packages, `Table`, `truncateDisplay`, and `padCell` allocate via `std.mem.Allocator`; always `defer tbl.deinit()` and `defer alloc.free(out)`.
- Mutating methods (`deinit`, `setHeaders`, `addRow`, `setAlign`, `setStyle`, `setMaxColWidth`) take `*_`; bind with `let tbl = Table.init(alloc)`.
- `setHeaders` and `addRow` store string slices; keep the underlying row/header strings alive until `tbl.render(alloc)` finishes.
- `setMaxColWidth(0)` disables truncation (default); `displayWidth` is zero-allocation.
