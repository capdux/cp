# @cap/enquirer

Zero-heap, pure CP interactive CLI prompt state machines for `ConfirmPrompt`, `SelectPrompt`, and `MultiSelectPrompt` (clean-room MIT implementation inspired by npm `enquirer` v2).

## Features

- **Zero heap allocation**: Fixed 16-choice tables with caller-owned render buffers.
- **Composable**: Reuses `@cap/picocolors` (`PicoFormatter`) for ANSI prompt symbol and active choice styling.
- **Complete prompt state machines**: `ConfirmPrompt` answer parsing, `SelectPrompt` wrap-around navigation skipping disabled items, and `MultiSelectPrompt` toggling.

## Usage

```cp
import { ConfirmPrompt, SelectPrompt, MultiSelectPrompt } from "@cap/enquirer";

let sel = SelectPrompt.new("Choose runtime");
* = sel.addChoice("CP Native", true);
* = sel.addChoice("Legacy", false);
* = sel.addChoice("WASM", true);
sel.moveDown(); // skips disabled "Legacy" -> lands on "WASM"
let picked = sel.submit();
```
