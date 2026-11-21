// Source: cp/cap/test/test_nix_production_deep.cp (Production deep tests for cap:nix)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NIX_MAX_DEPTH,
    NixTokenKind,
    NixToken,
    NixAttrBinding,
    NixIter,
    NixAttrIter,
    validateNixDelimiters,
} from "cap:nix";

test("1. NixIter tokenizes strings with ${...}, indented strings, paths, URIs, numbers, ellipsis, and comments", () => {
    const src =
        "# line comment\n" ++
        "/* block comment */\n" ++
        "{ pkgs ? import <nixpkgs> {}, ... }:\n" ++
        "let\n" ++
        "  msg = \"hello ${if true then \"nix\" else \"other\"}!\";\n" ++
        "  script = ''echo '''quoted''' ''$HOME ${pkgs.hello}/bin/hello'';\n" ++
        "  local = ./sub/module.nix;\n" ++
        "  home = https://nixos.org/channels/nixpkgs-unstable;\n" ++
        "  ver = 24.05;\n" ++
        "in pkgs.mkShell { build-inputs = [ local ]; }\n";

    let it = NixIter.init(src);

    let string_count: usize = 0;
    let indented_count: usize = 0;
    let path_count: usize = 0;
    let uri_count: usize = 0;
    let number_count: usize = 0;
    let ellipsis_count: usize = 0;
    let found_nixpkgs_path = false;
    let found_rel_path = false;
    let found_hyphen_ident = false;

    while (tok := it.next()) {
        expect_eq(tok.kind != NixTokenKind.invalid, true);
        if (tok.kind == NixTokenKind.string) string_count += 1;
        if (tok.kind == NixTokenKind.indented_string) indented_count += 1;
        if (tok.kind == NixTokenKind.path) {
            path_count += 1;
            if (std.mem.eql(u8, tok.lexeme, "<nixpkgs>")) found_nixpkgs_path = true;
            if (std.mem.eql(u8, tok.lexeme, "./sub/module.nix")) found_rel_path = true;
        }
        if (tok.kind == NixTokenKind.uri) uri_count += 1;
        if (tok.kind == NixTokenKind.number) number_count += 1;
        if (tok.kind == NixTokenKind.ellipsis) ellipsis_count += 1;
        if (tok.kind == NixTokenKind.ident && std.mem.eql(u8, tok.lexeme, "build-inputs")) {
            found_hyphen_ident = true;
        }
    }

    expect_eq(string_count, 1);
    expect_eq(indented_count, 1);
    expect_eq(path_count, 2);
    expect_eq(uri_count, 1);
    expect_eq(number_count, 1);
    expect_eq(ellipsis_count, 1);
    expect_eq(found_nixpkgs_path, true);
    expect_eq(found_rel_path, true);
    expect_eq(found_hyphen_ident, true);
});

test("2. NixAttrIter extracts top-level flake.nix bindings and ignores nested semicolons and inherit", () => {
    const flake =
        "# Flake definition\n" ++
        "{\n" ++
        "  description = \"CP stdlib flake\";\n" ++
        "  inputs.nixpkgs.url = \"github:NixOS/nixpkgs/nixos-unstable\";\n" ++
        "  inherit (builtins) currentSystem;\n" ++
        "  outputs = { self, nixpkgs, ... }: {\n" ++
        "    packages.x86_64-linux.default = nixpkgs.legacyPackages.x86_64-linux.hello;\n" ++
        "  };\n" ++
        "}\n";

    let it = NixAttrIter.init(flake);

    const b0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, b0.path, "description"), true);
    expect_eq(std.mem.eql(u8, b0.value, "\"CP stdlib flake\""), true);

    const b1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, b1.path, "inputs.nixpkgs.url"), true);
    expect_eq(std.mem.eql(u8, b1.value, "\"github:NixOS/nixpkgs/nixos-unstable\""), true);

    const b2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, b2.path, "outputs"), true);
    expect_eq(b2.value.len > 20, true);

    expect_eq(it.next() == null, true);
});

test("3. NixAttrIter supports bare attrset bodies, rec attrsets, and quoted path segments", () => {
    const bare =
        "pname = \"cap\";\n" ++
        "\"x86_64-linux\".default = ./default.nix;\n";

    let it1 = NixAttrIter.init(bare);
    const a0 = it1.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, a0.path, "pname"), true);
    expect_eq(std.mem.eql(u8, a0.value, "\"cap\""), true);

    const a1 = it1.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, a1.path, "\"x86_64-linux\".default"), true);
    expect_eq(std.mem.eql(u8, a1.value, "./default.nix"), true);
    expect_eq(it1.next() == null, true);

    const rec_set = "rec { version = \"1.0\"; name = \"cap-${version}\"; }";
    let it2 = NixAttrIter.init(rec_set);
    const r0 = it2.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r0.path, "version"), true);
    const r1 = it2.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.path, "name"), true);
    expect_eq(std.mem.eql(u8, r1.value, "\"cap-${version}\""), true);
    expect_eq(it2.next() == null, true);
});

test("4. validateNixDelimiters respects strings/comments/interpolations and enforces 64-level depth cap", () => {
    expect_eq(validateNixDelimiters("{ a = \"{[(\"; b = ''[({ ''; # {\n /* [ */ c = [ (1 + 2) ]; }"), true);
    expect_eq(validateNixDelimiters("{ x = \"outer ${ { k = \"inner }\"; }.k }\"; }"), true);

    // Mismatches and unclosed structures
    expect_eq(validateNixDelimiters("{ a = [1, 2); }"), false);
    expect_eq(validateNixDelimiters("{ a = [1, 2];"), false);
    expect_eq(validateNixDelimiters("{ a = 1; }} "), false);
    expect_eq(validateNixDelimiters("{ a = \"unclosed ${ 1 + 2\"; }"), false);
    expect_eq(validateNixDelimiters("{ a = ''unclosed; }"), false);
    expect_eq(validateNixDelimiters("{ /* unclosed }"), false);

    // Exact 64-level nesting passes; 65-level nesting is rejected
    let ok_buf: [128]u8 = std.mem.zeroes([128]u8);
    let i: usize = 0;
    while (i < NIX_MAX_DEPTH) {
        ok_buf[i] = '{';
        ok_buf[NIX_MAX_DEPTH * 2 - 1 - i] = '}';
        i += 1;
    }
    expect_eq(validateNixDelimiters(ok_buf[0..128]), true);

    let deep_buf: [130]u8 = std.mem.zeroes([130]u8);
    i = 0;
    while (i < NIX_MAX_DEPTH + 1) {
        deep_buf[i] = '{';
        deep_buf[(NIX_MAX_DEPTH + 1) * 2 - 1 - i] = '}';
        i += 1;
    }
    expect_eq(validateNixDelimiters(deep_buf[0..130]), false);
});

test("5. Malformed lexical tokens and truncated attrset bindings fail cleanly without looping", () => {
    let it1 = NixIter.init("\"unclosed string");
    expect_eq((it1.next() ?? unreachable).kind == NixTokenKind.invalid, true);

    let it2 = NixIter.init("''unclosed indented");
    expect_eq((it2.next() ?? unreachable).kind == NixTokenKind.invalid, true);

    let it3 = NixIter.init("/* unclosed block comment");
    expect_eq((it3.next() ?? unreachable).kind == NixTokenKind.invalid, true);

    let it4 = NixIter.init("1e+");
    expect_eq((it4.next() ?? unreachable).kind == NixTokenKind.invalid, true);

    // Truncated attribute without terminating semicolon returns null cleanly
    let attr_it = NixAttrIter.init("{ incomplete = 123 }");
    expect_eq(attr_it.next() == null, true);
});
