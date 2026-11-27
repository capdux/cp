// Type definitions for CP

import { trim, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { PicoFormatter } from "../../picocolors/src/index.cp";
export class ConfirmPrompt {
    message: string,
    initial: bool,
    fn new(message: string, initial: bool): ConfirmPrompt;
    fn parseAnswer(input: string): bool?;
    fn render(use_color: bool, out: []u8): usize;
}

export class SelectChoice {
    label: string,
    enabled: bool,
}

export class SelectPrompt {
    message: string,
    choices: [16]SelectChoice,
    choice_count: usize,
    cursor: usize,
    submitted: bool,
    fn new(message: string): SelectPrompt;
    fn addChoice(_: auto, label: string, enabled: bool): bool;
    fn moveDown(_: auto);
    fn moveUp(_: auto);
    fn submit(_: auto): string?;
    fn render(use_color: bool, out: []u8): usize;
}

export class MultiSelectPrompt {
    message: string,
    choices: [16]string,
    selected: [16]bool,
    choice_count: usize,
    fn new(message: string): MultiSelectPrompt;
    fn addChoice(_: auto, label: string): bool;
    fn toggle(_: auto, idx: usize): bool;
    fn selectedCount(): usize;
    fn isSelected(idx: usize): bool;
    fn render(use_color: bool, out: []u8): usize;
}

