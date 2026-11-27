// cap/test/testCapPluralize.cp
// Comprehensive unit tests for @cap/pluralize:
// regular plural/singular rules, 56-pair irregular dictionary, uncountable nouns,
// case preservation (lower / Title / UPPER), pluralizeCount, and ordinal boundaries.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    plural,
    singular,
    isPlural,
    isSingular,
    isUncountable,
    pluralizeCount,
    ordinal,
} from "../../packages/pluralize/src/index.cp";

fn eq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn main() {
    test("1. @cap/pluralize — Regular English Plural & Singular Rules", () => {
        let buf: [64]u8 = undefined;

        // Default +s / -s
        expect_eq(eq(plural("cat", &buf), "cats"), true);
        expect_eq(eq(singular("cats", &buf), "cat"), true);
        expect_eq(eq(plural("item", &buf), "items"), true);
        expect_eq(eq(singular("items", &buf), "item"), true);

        // Sibilants (-s, -ss, -sh, -ch, -x, -z -> +es)
        expect_eq(eq(plural("bus", &buf), "buses"), true);
        expect_eq(eq(singular("buses", &buf), "bus"), true);
        expect_eq(eq(plural("class", &buf), "classes"), true);
        expect_eq(eq(singular("classes", &buf), "class"), true);
        expect_eq(eq(plural("dish", &buf), "dishes"), true);
        expect_eq(eq(singular("dishes", &buf), "dish"), true);
        expect_eq(eq(plural("watch", &buf), "watches"), true);
        expect_eq(eq(singular("watches", &buf), "watch"), true);
        expect_eq(eq(plural("box", &buf), "boxes"), true);
        expect_eq(eq(singular("boxes", &buf), "box"), true);
        expect_eq(eq(plural("buzz", &buf), "buzzes"), true);
        expect_eq(eq(singular("buzzes", &buf), "buzz"), true);

        // Consonant+y -> -ies vs Vowel+y -> +s
        expect_eq(eq(plural("city", &buf), "cities"), true);
        expect_eq(eq(singular("cities", &buf), "city"), true);
        expect_eq(eq(plural("baby", &buf), "babies"), true);
        expect_eq(eq(singular("babies", &buf), "baby"), true);
        expect_eq(eq(plural("boy", &buf), "boys"), true);
        expect_eq(eq(singular("boys", &buf), "boy"), true);
        expect_eq(eq(plural("key", &buf), "keys"), true);
        expect_eq(eq(singular("keys", &buf), "key"), true);

        // Idempotence & predicates
        expect_eq(eq(plural("cats", &buf), "cats"), true);
        expect_eq(eq(singular("cat", &buf), "cat"), true);
        expect_eq(isPlural("boxes"), true);
        expect_eq(isSingular("boxes"), false);
        expect_eq(isSingular("box"), true);
        expect_eq(isPlural("box"), false);
    });

    test("2. @cap/pluralize — Irregular Dictionary (Human, -f/-fe, Latin/Greek, -o)", () => {
        let buf: [64]u8 = undefined;

        expect_eq(eq(plural("child", &buf), "children"), true);
        expect_eq(eq(singular("children", &buf), "child"), true);
        expect_eq(eq(plural("person", &buf), "people"), true);
        expect_eq(eq(singular("people", &buf), "person"), true);
        expect_eq(eq(plural("mouse", &buf), "mice"), true);
        expect_eq(eq(singular("mice", &buf), "mouse"), true);
        expect_eq(eq(plural("ox", &buf), "oxen"), true);
        expect_eq(eq(singular("oxen", &buf), "ox"), true);

        // -f / -fe irregulars
        expect_eq(eq(plural("leaf", &buf), "leaves"), true);
        expect_eq(eq(singular("leaves", &buf), "leaf"), true);
        expect_eq(eq(plural("knife", &buf), "knives"), true);
        expect_eq(eq(singular("knives", &buf), "knife"), true);
        expect_eq(eq(plural("wolf", &buf), "wolves"), true);
        expect_eq(eq(singular("wolves", &buf), "wolf"), true);
        expect_eq(eq(plural("thief", &buf), "thieves"), true);
        expect_eq(eq(singular("thieves", &buf), "thief"), true);

        // Latin / Greek irregulars
        expect_eq(eq(plural("cactus", &buf), "cacti"), true);
        expect_eq(eq(singular("cacti", &buf), "cactus"), true);
        expect_eq(eq(plural("analysis", &buf), "analyses"), true);
        expect_eq(eq(singular("analyses", &buf), "analysis"), true);
        expect_eq(eq(plural("criterion", &buf), "criteria"), true);
        expect_eq(eq(singular("criteria", &buf), "criterion"), true);
        expect_eq(eq(plural("datum", &buf), "data"), true);
        expect_eq(eq(singular("data", &buf), "datum"), true);
        expect_eq(eq(plural("matrix", &buf), "matrices"), true);
        expect_eq(eq(singular("matrices", &buf), "matrix"), true);
        expect_eq(eq(plural("vertex", &buf), "vertices"), true);
        expect_eq(eq(singular("vertices", &buf), "vertex"), true);
        expect_eq(eq(plural("alumnus", &buf), "alumni"), true);
        expect_eq(eq(singular("alumni", &buf), "alumnus"), true);

        // -o & misc irregulars
        expect_eq(eq(plural("quiz", &buf), "quizzes"), true);
        expect_eq(eq(singular("quizzes", &buf), "quiz"), true);
        expect_eq(eq(plural("hero", &buf), "heroes"), true);
        expect_eq(eq(singular("heroes", &buf), "hero"), true);
        expect_eq(eq(plural("potato", &buf), "potatoes"), true);
        expect_eq(eq(singular("potatoes", &buf), "potato"), true);
        expect_eq(eq(plural("die", &buf), "dice"), true);
        expect_eq(eq(singular("dice", &buf), "die"), true);
    });

    test("3. @cap/pluralize — Uncountable Nouns", () => {
        let buf: [64]u8 = undefined;

        const words: [10][]const u8 = [
            "information", "equipment", "money", "species", "series",
            "sheep", "deer", "software", "metadata", "news",
        ];
        let i: usize = 0;
        while (i < words.len) : (i += 1) {
            const w = words[i];
            expect_eq(isUncountable(w), true);
            expect_eq(isPlural(w), true);
            expect_eq(isSingular(w), true);
            expect_eq(eq(plural(w, &buf), w), true);
            expect_eq(eq(singular(w, &buf), w), true);
        }
        expect_eq(isUncountable("Information"), true);
        expect_eq(isUncountable("apple"), false);
    });

    test("4. @cap/pluralize — Case Style Preservation (lower / Title / UPPER)", () => {
        let buf: [64]u8 = undefined;

        expect_eq(eq(plural("child", &buf), "children"), true);
        expect_eq(eq(plural("Child", &buf), "Children"), true);
        expect_eq(eq(plural("CHILD", &buf), "CHILDREN"), true);

        expect_eq(eq(singular("Children", &buf), "Child"), true);
        expect_eq(eq(singular("CHILDREN", &buf), "CHILD"), true);

        expect_eq(eq(plural("City", &buf), "Cities"), true);
        expect_eq(eq(plural("CITY", &buf), "CITIES"), true);
        expect_eq(eq(singular("CITIES", &buf), "CITY"), true);

        expect_eq(eq(plural("Box", &buf), "Boxes"), true);
        expect_eq(eq(plural("BOX", &buf), "BOXES"), true);
    });

    test("5. @cap/pluralize — pluralizeCount Formatting", () => {
        let buf: [64]u8 = undefined;

        expect_eq(eq(pluralizeCount("item", 0, &buf), "0 items"), true);
        expect_eq(eq(pluralizeCount("item", 1, &buf), "1 item"), true);
        expect_eq(eq(pluralizeCount("item", 3, &buf), "3 items"), true);
        expect_eq(eq(pluralizeCount("items", 1, &buf), "1 item"), true);
        expect_eq(eq(pluralizeCount("child", 1, &buf), "1 child"), true);
        expect_eq(eq(pluralizeCount("child", 5, &buf), "5 children"), true);
        expect_eq(eq(pluralizeCount("sheep", 4, &buf), "4 sheep"), true);
        expect_eq(eq(pluralizeCount("item", -1, &buf), "-1 item"), true);
        expect_eq(eq(pluralizeCount("item", -2, &buf), "-2 items"), true);
    });

    test("6. @cap/pluralize — Ordinal Formatting Including 11/12/13 Boundaries", () => {
        let buf: [32]u8 = undefined;

        expect_eq(eq(ordinal(0, &buf), "0th"), true);
        expect_eq(eq(ordinal(1, &buf), "1st"), true);
        expect_eq(eq(ordinal(2, &buf), "2nd"), true);
        expect_eq(eq(ordinal(3, &buf), "3rd"), true);
        expect_eq(eq(ordinal(4, &buf), "4th"), true);
        expect_eq(eq(ordinal(11, &buf), "11th"), true);
        expect_eq(eq(ordinal(12, &buf), "12th"), true);
        expect_eq(eq(ordinal(13, &buf), "13th"), true);
        expect_eq(eq(ordinal(21, &buf), "21st"), true);
        expect_eq(eq(ordinal(22, &buf), "22nd"), true);
        expect_eq(eq(ordinal(23, &buf), "23rd"), true);
        expect_eq(eq(ordinal(111, &buf), "111th"), true);
        expect_eq(eq(ordinal(112, &buf), "112th"), true);
        expect_eq(eq(ordinal(113, &buf), "113th"), true);
        expect_eq(eq(ordinal(121, &buf), "121st"), true);
        expect_eq(eq(ordinal(-3, &buf), "-3rd"), true);
    });
}
