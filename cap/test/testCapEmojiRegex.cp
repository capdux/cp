import { test, expect_eq } from "cap:test";
import {
    EmojiSpan,
    isEmojiCodePoint,
    findNextEmoji,
    countEmojis,
    containsEmoji,
    stripEmojis,
} from "../../packages/@cap/emoji-regex/src/index.cp";

fn main() {
    test("1. @cap/emoji-regex — isEmojiCodePoint covers core Unicode blocks", () => {
        expect_eq(isEmojiCodePoint(0x1F600), true);  // Grinning face
        expect_eq(isEmojiCodePoint(0x1F680), true);  // Rocket
        expect_eq(isEmojiCodePoint(0x2764), true);   // Heavy black heart
        expect_eq(isEmojiCodePoint(0x2600), true);   // Sun
        expect_eq(isEmojiCodePoint(0x1F1FA), true);  // Regional Indicator U
        expect_eq(isEmojiCodePoint(0x1F3FD), true);  // Medium skin tone
        expect_eq(isEmojiCodePoint(0x2B50), true);   // Star
        expect_eq(isEmojiCodePoint(0x0041), false);  // 'A'
        expect_eq(isEmojiCodePoint(0x4E2D), false);  // '中'
    });

    test("2. @cap/emoji-regex — Plain ASCII/CJK text has no emojis", () => {
        const plain: string = "Hello CP 12345 中文测试";
        expect_eq(containsEmoji(plain), false);
        expect_eq(countEmojis(plain), 0);
        expect_eq(findNextEmoji(plain, 0) == null, true);
    });

    test("3. @cap/emoji-regex — Single emoji and VS16 heart sequence", () => {
        // "hi 😀 ❤️!"
        const text: string = "hi \xF0\x9F\x98\x80 \xE2\x9D\xA4\xEF\xB8\x8F!";
        expect_eq(containsEmoji(text), true);
        expect_eq(countEmojis(text), 2);

        const s1: EmojiSpan = findNextEmoji(text, 0).?;
        expect_eq(s1.start, 3);
        expect_eq(s1.end, 7);

        const s2: EmojiSpan = findNextEmoji(text, s1.end).?;
        expect_eq(s2.start, 8);
        expect_eq(s2.end, 14); // 3-byte heart + 3-byte VS16
    });

    test("4. @cap/emoji-regex — ZWJ family sequence and skin tone modifier count as single spans", () => {
        // Woman Technologist: 👩 (F0 9F 91 A9) + ZWJ (E2 80 8D) + 💻 (F0 9F 92 BB)
        const zwj: string = "dev:\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB!";
        expect_eq(countEmojis(zwj), 1);
        const zs: EmojiSpan = findNextEmoji(zwj, 0).?;
        expect_eq(zs.start, 4);
        expect_eq(zs.end, 15); // 4 + 3 + 4 = 11 bytes

        // Thumbs up + Medium skin tone: 👍 (F0 9F 91 8D) + 🏽 (F0 9F 8F BD)
        const skin: string = "ok \xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD";
        expect_eq(countEmojis(skin), 1);
        const ss: EmojiSpan = findNextEmoji(skin, 0).?;
        expect_eq(ss.start, 3);
        expect_eq(ss.end, 11); // 4 + 4 = 8 bytes
    });

    test("5. @cap/emoji-regex — Regional Indicator flag pair and Keycap sequence", () => {
        // US flag: 🇺 (F0 9F 87 BA) + 🇸 (F0 9F 87 B8)
        // Keycap 1: '1' (31) + FE0F (EF B8 8F) + 20E3 (E2 83 A3)
        const mixed: string = "flag:\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8 key:1\xEF\xB8\x8F\xE2\x83\xA3.";
        expect_eq(countEmojis(mixed), 2);

        const fspan: EmojiSpan = findNextEmoji(mixed, 0).?;
        expect_eq(fspan.start, 5);
        expect_eq(fspan.end, 13);

        const kspan: EmojiSpan = findNextEmoji(mixed, fspan.end).?;
        expect_eq(kspan.start, 18);
        expect_eq(kspan.end, 25); // 1 + 3 + 3 = 7 bytes
    });

    test("6. @cap/emoji-regex — stripEmojis removes all emojis cleanly", () => {
        const src: string = "A\xF0\x9F\x9A\x80B\xE2\x9D\xA4\xEF\xB8\x8FC\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8D";
        let buf: [16]u8 = undefined;
        const n: usize = stripEmojis(src, &buf);
        expect_eq(n, 4);
        expect_eq(buf[0], 'A');
        expect_eq(buf[1], 'B');
        expect_eq(buf[2], 'C');
        expect_eq(buf[3], 'D');
    });
}
