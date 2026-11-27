// Type definitions for CP

import { strEq, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { escapeHtml } from "../../escape-html/src/index.cp";
export fn rhWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn isVoidElement(tag: string): bool;
export class HastAttr {
    name: [48]u8,
    name_len: usize,
    value: [192]u8,
    value_len: usize,
    next_attr: i32,
}

export class HastNode {
    kind: u8,
    text: [256]u8,
    text_len: usize,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
    first_attr: i32,
    last_attr: i32,
}

export fn rhMakeNode(kind: u8, text: string): HastNode;
export fn rhSerializeHtml(nodes: &[]HastNode, attrs: &[]HastAttr, node_id: usize, out: []u8, pos: usize, depth: usize): usize;
export fn rhExtractText(nodes: &[]HastNode, node_id: usize, out: []u8, pos: usize, depth: usize): usize;
export class HastTree {
    nodes: [64]HastNode,
    node_len: usize,
    attrs: [64]HastAttr,
    attr_len: usize,
    fn new(): HastTree;
    fn addElement(_: auto, parent_id: usize, tag: string): i32;
    fn addText(_: auto, parent_id: usize, text: string): i32;
    fn addComment(_: auto, parent_id: usize, comment: string): i32;
    fn addDoctype(_: auto, parent_id: usize): i32;
    fn setAttr(_: auto, node_id: usize, name: string, value: string): bool;
    fn toHtml(out: []u8): usize;
    fn toText(out: []u8): usize;
    fn nodeCount(): usize;
    fn countByTag(tag: string): usize;
}

