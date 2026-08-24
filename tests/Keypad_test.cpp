#include <gtest/gtest.h>
#include "../src/Keypad.h"

TEST(KeypadTest, KeysStartReleased) {
    Keypad keypad;
    for (uint8_t key{0}; key < Keypad::KEY_COUNT; ++key) {
        EXPECT_FALSE(keypad.is_key_down(key));
    }
}

TEST(KeypadTest, PressSetsKeyDown) {
    Keypad keypad;
    keypad.press(0x5);
    EXPECT_TRUE(keypad.is_key_down(0x5));
}

TEST(KeypadTest, ReleaseClearsKeyDown) {
    Keypad keypad;
    keypad.press(0xA);
    keypad.release(0xA);
    EXPECT_FALSE(keypad.is_key_down(0xA));
}

TEST(KeypadTest, PressAndReleaseOnlyAffectTargetKey) {
    Keypad keypad;
    keypad.press(0x3);
    EXPECT_TRUE(keypad.is_key_down(0x3));
    for (uint8_t key{0}; key < Keypad::KEY_COUNT; ++key) {
        if (key == 0x3) continue;
        EXPECT_FALSE(keypad.is_key_down(key));
    }
}

TEST(KeypadTest, OutOfRangeKeyIsIgnoredAndReturnsFalse) {
    Keypad keypad;
    keypad.press(0x10); // one past the highest valid key (0xF)
    EXPECT_FALSE(keypad.is_key_down(0x10));
    EXPECT_FALSE(keypad.is_key_down(0xFF));
}
