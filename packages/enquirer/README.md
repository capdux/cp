[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# enquirer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:enquirer` ([https://github.com/enquirer/enquirer](https://github.com/enquirer/enquirer))

Zero-heap, pure CP interactive CLI prompt state machines for `ConfirmPrompt`, `SelectPrompt`, and `MultiSelectPrompt` (clean-room MIT implementation inspired by npm `enquirer` v2).

## Features

- **Zero heap allocation**: Fixed 16-choice tables with caller-owned render buffers.
- **Composable**: Reuses `picocolors` (`PicoFormatter`) for ANSI prompt symbol and active choice styling.
- **Complete prompt state machines**: `ConfirmPrompt` answer parsing, `SelectPrompt` wrap-around navigation skipping disabled items, and `MultiSelectPrompt` toggling.

## Usage

```cp
import { ConfirmPrompt, SelectPrompt, MultiSelectPrompt } from "enquirer";

let sel = SelectPrompt.new("Choose runtime");
* = sel.addChoice("CP Native", true);
* = sel.addChoice("Legacy", false);
* = sel.addChoice("WASM", true);
sel.moveDown(); // skips disabled "Legacy" -> lands on "WASM"
let picked = sel.submit();
```
