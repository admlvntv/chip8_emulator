#include <gtest/gtest.h>
#include "../src/CPU.h"
#include "../src/Display.h"
#include "../src/Keypad.h"
#include <filesystem>
#include <fstream>
#include <vector>

class CPUForTesting : public CPU {
public:
    using CPU::m_memory;
    using CPU::m_V;
    using CPU::m_I;
    using CPU::m_pc;
    using CPU::m_stack;
    using CPU::m_delay_timer;
    using CPU::m_sound_timer;
    using CPU::fetch;
    using CPU::execute;
    using CPU::MEMORY_SIZE;
    using CPU::ROM_START_ADDRESS;
    using CPU::STACK_DEPTH;
    using CPU::FONT_START_ADDRESS;
    using CPU::FONT_CHAR_SIZE;
};

class MockDisplay : public Display {
public:
    void render() const override {}

    // Helper to check pixel state in tests
    uint8_t get_pixel_at(int x, int y) const {
        return m_pixels[y * WIDTH + x];
    }

    bool is_cleared() const {
        for (auto pixel : m_pixels) {
            if (pixel != 0) return false;
        }
        return true;
    }
};

class CPUTest : public testing::Test {
protected:
    CPUForTesting cpu;
    MockDisplay display;
    Keypad keypad;

    void set_memory(uint16_t address, const std::vector<uint8_t>& data) {
        for (size_t i{0}; i < data.size(); ++i) {
            cpu.m_memory[address + i] = data[i];
        }
    }
};

TEST_F(CPUTest, FontsetLoadedAtInitialization) {
    // First char '0': 0xF0, 0x90, 0x90, 0x90, 0xF0
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS], 0xF0);
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 1], 0x90);
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 2], 0x90);
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 3], 0x90);
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 4], 0xF0);

    // Last char 'F': 0xF0, 0x80, 0xF0, 0x80, 0x80
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 0xF * cpu.FONT_CHAR_SIZE], 0xF0);
    EXPECT_EQ(cpu.m_memory[cpu.FONT_START_ADDRESS + 0xF * cpu.FONT_CHAR_SIZE + 4], 0x80);
}

TEST_F(CPUTest, FetchConstructsCorrectOpcodeAndAdvancesPC) {
    uint16_t start_pc{cpu.m_pc};
    set_memory(start_pc, {0xA2, 0xF0});
    
    uint16_t opcode{cpu.fetch()};
    
    EXPECT_EQ(opcode, 0xA2F0);
    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode00E0ClearsDisplay) {
    display.write_pixel(0, 0);
    EXPECT_FALSE(display.is_cleared());
    
    cpu.execute(0x00E0, display, keypad);
    
    EXPECT_TRUE(display.is_cleared());
}

TEST_F(CPUTest, Opcode1NNNJumpsToAddress) {
    cpu.execute(0x1250, display, keypad);
    EXPECT_EQ(cpu.m_pc, 0x250);
}

TEST_F(CPUTest, Opcode00EEReturnsFromSubroutine) {
    cpu.m_stack.push(0x350);

    cpu.execute(0x00EE, display, keypad);

    EXPECT_EQ(cpu.m_pc, 0x350);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode00EEThrowsWhenStackIsEmpty) {
    EXPECT_TRUE(cpu.m_stack.empty());
    EXPECT_THROW(cpu.execute(0x00EE, display, keypad), std::out_of_range);
}

TEST_F(CPUTest, Opcode2NNNPushesPCAndJumpsToAddress) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2300, display, keypad);

    EXPECT_EQ(cpu.m_pc, 0x300);
    ASSERT_FALSE(cpu.m_stack.empty());
    EXPECT_EQ(cpu.m_stack.top(), start_pc);
}

TEST_F(CPUTest, Opcode2NNNThrowsOnInvalidAddress) {
    EXPECT_THROW(cpu.execute(0x2100, display, keypad), std::invalid_argument);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode2NNNThrowsOnStackOverflow) {
    // Fill the stack to STACK_DEPTH
    for (size_t i{0}; i < cpu.STACK_DEPTH; ++i) {
        cpu.execute(0x2300, display, keypad);
    }
    EXPECT_EQ(cpu.m_stack.size(), cpu.STACK_DEPTH);

    // One more call should overflow rather than grow the stack unbounded
    EXPECT_THROW(cpu.execute(0x2300, display, keypad), std::overflow_error);
    EXPECT_EQ(cpu.m_stack.size(), cpu.STACK_DEPTH);
}

TEST_F(CPUTest, Opcode2NNNThenOpcode00EERoundTripsToCallSite) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2400, display, keypad); // Call 0x400
    EXPECT_EQ(cpu.m_pc, 0x400);

    cpu.execute(0x00EE, display, keypad); // Return
    EXPECT_EQ(cpu.m_pc, start_pc);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode2NNNSupportsNestedCalls) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2300, display, keypad); // Call 0x300, pushes start_pc
    uint16_t after_first_call{cpu.m_pc};
    cpu.execute(0x2400, display, keypad); // Call 0x400, pushes after_first_call
    EXPECT_EQ(cpu.m_pc, 0x400);
    EXPECT_EQ(cpu.m_stack.size(), 2);

    cpu.execute(0x00EE, display, keypad); // Return to after_first_call
    EXPECT_EQ(cpu.m_pc, after_first_call);

    cpu.execute(0x00EE, display, keypad); // Return to start_pc
    EXPECT_EQ(cpu.m_pc, start_pc);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode3XNNSkipsWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x31;

    cpu.execute(0x3331, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode3XNNDoesNotSkipWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x32;

    cpu.execute(0x3331, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode4XNNDoesNotSkipWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[4] = 0x41;

    cpu.execute(0x4441, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode4XNNSkipsWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[4] = 0x42;

    cpu.execute(0x4441, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode5XY0SkipsWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x51;
    cpu.m_V[2] = 0x51;

    cpu.execute(0x5120, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode5XY0DoesNotSkipWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x51;
    cpu.m_V[2] = 0x52;

    cpu.execute(0x5120, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode5XY0ThrowsOnInvalidSubOpcode) {
    EXPECT_THROW(cpu.execute(0x5121, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, Opcode6XNNSetsRegister) {
    cpu.execute(0x62FF, display, keypad);
    EXPECT_EQ(cpu.m_V[2], 0xFF);
}

TEST_F(CPUTest, Opcode7XNNAddsToRegister) {
    cpu.m_V[3] = 0x10;
    cpu.execute(0x7305, display, keypad);
    EXPECT_EQ(cpu.m_V[3], 0x15);
    
    // Test wrap around
    cpu.m_V[3] = 0xFE;
    cpu.execute(0x7304, display, keypad);
    EXPECT_EQ(cpu.m_V[3], 0x02);
}

TEST_F(CPUTest, Opcode8XY0SetsVxToVy) {
    cpu.m_V[1] = 0x11;
    cpu.m_V[2] = 0x22;

    cpu.execute(0x8120, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x22);
}

TEST_F(CPUTest, Opcode8XY1SetsVxToVxORVy) {
    cpu.m_V[1] = 0b1010;
    cpu.m_V[2] = 0b0101;

    cpu.execute(0x8121, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b1111);
}

TEST_F(CPUTest, Opcode8XY2SetsVxToVxANDVy) {
    cpu.m_V[1] = 0b1100;
    cpu.m_V[2] = 0b1010;

    cpu.execute(0x8122, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b1000);
}

TEST_F(CPUTest, Opcode8XY3SetsVxToVxXORVy) {
    cpu.m_V[1] = 0b1100;
    cpu.m_V[2] = 0b1010;

    cpu.execute(0x8123, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b0110);
}

TEST_F(CPUTest, Opcode8XY4AddsVyToVxWithoutCarry) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8124, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x15);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XY4AddsVyToVxWithCarry) {
    cpu.m_V[1] = 0xFE;
    cpu.m_V[2] = 0x04;

    cpu.execute(0x8124, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x02); // wraps around mod 256
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XY5SubtractsVyFromVxWithoutBorrow) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8125, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x0B);
    EXPECT_EQ(cpu.m_V[0xF], 1); // VX >= VY, no borrow
}

TEST_F(CPUTest, Opcode8XY5SubtractsVyFromVxWithBorrow) {
    cpu.m_V[1] = 0x05;
    cpu.m_V[2] = 0x10;

    cpu.execute(0x8125, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0xF5); // wraps around
    EXPECT_EQ(cpu.m_V[0xF], 0); // VX < VY, borrow occurred
}

TEST_F(CPUTest, Opcode8XY6ShiftsVyRightIntoVxAndSetsVFToShiftedOutBit) {
    cpu.m_V[2] = 0b00000011; // least significant bit is 1

    cpu.execute(0x8126, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b00000001);
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XY6ClearsVFWhenShiftedOutBitIsZero) {
    cpu.m_V[2] = 0b00000010; // least significant bit is 0

    cpu.execute(0x8126, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b00000001);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XY7SetsVxToVyMinusVxWithoutBorrow) {
    cpu.m_V[1] = 0x05;
    cpu.m_V[2] = 0x10;

    cpu.execute(0x8127, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x0B);
    EXPECT_EQ(cpu.m_V[0xF], 1); // VY >= VX, no borrow
}

TEST_F(CPUTest, Opcode8XY7SetsVxToVyMinusVxWithBorrow) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8127, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0xF5); // wraps around
    EXPECT_EQ(cpu.m_V[0xF], 0); // VY < VX, borrow occurred
}

TEST_F(CPUTest, Opcode8XYEShiftsVyLeftIntoVxAndSetsVFToShiftedOutBit) {
    cpu.m_V[2] = 0b11000000; // most significant bit is 1

    cpu.execute(0x812E, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b10000000);
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XYEClearsVFWhenShiftedOutBitIsZero) {
    cpu.m_V[2] = 0b01000000; // most significant bit is 0

    cpu.execute(0x812E, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0b10000000);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XYNThrowsOnUnknownSubOpcode) {
    EXPECT_THROW(cpu.execute(0x8128, display, keypad), std::invalid_argument);
    EXPECT_THROW(cpu.execute(0x8129, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, Opcode9XY0SkipsWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x91;
    cpu.m_V[2] = 0x92;

    cpu.execute(0x9120, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode9XY0DoesNotSkipWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x91;
    cpu.m_V[2] = 0x91;

    cpu.execute(0x9120, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode9XY0ThrowsOnInvalidSubOpcode) {
    EXPECT_THROW(cpu.execute(0x9121, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, OpcodeANNNSetsIRegister) {
    cpu.execute(0xAABC, display, keypad);
    EXPECT_EQ(cpu.m_I, 0xABC);
}

TEST_F(CPUTest, OpcodeBNNNJumpsToAddressPlusV0) {
    cpu.m_V[0] = 0x10;
    cpu.execute(0xB300, display, keypad);
    EXPECT_EQ(cpu.m_pc, 0x310);
}

TEST_F(CPUTest, OpcodeCXNNMasksRandomValueWithNN) {
    // NN = 0x00 forces the result to 0
    cpu.execute(0xC500, display, keypad);
    EXPECT_EQ(cpu.m_V[5], 0x00);

    // NN = 0x0F should always clear the upper nibble
    for (int i{0}; i < 100; ++i) {
        cpu.execute(0xC50F, display, keypad);
        EXPECT_EQ(cpu.m_V[5] & 0xF0, 0x00);
    }

    // NN = 0xF0 should always clear the lower nibble
    for (int i{0}; i < 100; ++i) {
        cpu.execute(0xC5F0, display, keypad);
        EXPECT_EQ(cpu.m_V[5] & 0x0F, 0x00);
    }
}

TEST_F(CPUTest, OpcodeCXNNProducesVaryingValues) {
    std::vector<uint8_t> results;
    for (int i{0}; i < 100; ++i) {
        cpu.execute(0xC6FF, display, keypad);
        results.push_back(cpu.m_V[6]);
    }

    bool all_same{true};
    for (uint8_t value : results) {
        if (value != results[0]) {
            all_same = false;
            break;
        }
    }
    EXPECT_FALSE(all_same);
}

TEST_F(CPUTest, OpcodeDXYNDrawsSpriteAndSetsCollision) {
    // Set I to a simple sprite (single pixel at top left)
    cpu.m_I = 0x300;
    set_memory(0x300, {0x80}); // 10000000
    
    cpu.m_V[0] = 0;
    cpu.m_V[1] = 0;
    
    // Draw it
    cpu.execute(0xD011, display, keypad);
    EXPECT_EQ(display.get_pixel_at(0, 0), 1);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    
    // Draw it again at same position, should collision and clear
    cpu.execute(0xD011, display, keypad);
    EXPECT_EQ(display.get_pixel_at(0, 0), 0);
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, OpcodeDXYNHandlesWrappingAndClipping) {
    // Define a 2x2 sprite (all pixels on) in memory
    cpu.m_I = 0x400;
    set_memory(0x400, {0xC0, 0xC0}); // 11000000, 11000000
    
    // Test Case 1: Partial clipping at the right and bottom edges
    // The pixels at (64, 31), (63, 32), and (64, 32) should be clipped.
    cpu.m_V[0] = Display::WIDTH - 1;
    cpu.m_V[1] = Display::HEIGHT - 1;

    cpu.execute(0xD012, display, keypad);
    
    // Only the top-left pixel of the sprite at (63, 31) should be drawn
    EXPECT_EQ(display.get_pixel_at(Display::WIDTH - 1, Display::HEIGHT - 1), 1);
    
    // Test Case 2: Coordinate wrapping
    // The STARTING position wraps
    // Set position to (66, 34), which should wrap to (2, 2)
    display.clear();
    cpu.m_V[0] = Display::WIDTH + 2;
    cpu.m_V[1] = Display::HEIGHT + 2;
    
    cpu.execute(0xD012, display, keypad);
    
    // Check that sprite was drawn at the wrapped coordinates (2, 2)
    EXPECT_EQ(display.get_pixel_at(2, 2), 1);
    EXPECT_EQ(display.get_pixel_at(3, 2), 1);
    EXPECT_EQ(display.get_pixel_at(2, 3), 1);
    EXPECT_EQ(display.get_pixel_at(3, 3), 1);
}

TEST_F(CPUTest, OpcodeEX9ESkipsWhenKeyIsDown) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x5;
    keypad.press(0x5);

    cpu.execute(0xE39E, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, OpcodeEX9EDoesNotSkipWhenKeyIsUp) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x5;
    keypad.release(0x5);

    cpu.execute(0xE39E, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, OpcodeEXA1SkipsWhenKeyIsUp) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x5;
    keypad.release(0x5);

    cpu.execute(0xE3A1, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, OpcodeEXA1DoesNotSkipWhenKeyIsDown) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x5;
    keypad.press(0x5);

    cpu.execute(0xE3A1, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, OpcodeEXNNThrowsOnUnknownSubOpcode) {
    EXPECT_THROW(cpu.execute(0xE000, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, OpcodeFX07SetsVxToDelayTimer) {
    cpu.m_delay_timer = 0x42;
    cpu.execute(0xF107, display, keypad);
    EXPECT_EQ(cpu.m_V[1], 0x42);
}

TEST_F(CPUTest, OpcodeFX0AStoresKeyAndAdvancesWhenAKeyIsDown) {
    uint16_t start_pc{cpu.m_pc};
    keypad.press(0x7);

    cpu.execute(0xF10A, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x7);
    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, OpcodeFX0ABlocksPCWhenNoKeyIsDown) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0xF10A, display, keypad);

    EXPECT_EQ(cpu.m_pc, start_pc - 2);
}

TEST_F(CPUTest, OpcodeFX0APicksLowestIndexKeyWhenMultipleAreDown) {
    keypad.press(0xB);
    keypad.press(0x2);

    cpu.execute(0xF10A, display, keypad);

    EXPECT_EQ(cpu.m_V[1], 0x2);
}

TEST_F(CPUTest, OpcodeFX15SetsDelayTimerToVx) {
    cpu.m_V[2] = 0x43;
    cpu.execute(0xF215, display, keypad);
    EXPECT_EQ(cpu.m_delay_timer, 0x43);
}

TEST_F(CPUTest, OpcodeFX18SetsSoundTimerToVx) {
    cpu.m_V[2] = 0x43;
    cpu.execute(0xF218, display, keypad);
    EXPECT_EQ(cpu.m_sound_timer, 0x43);
}

TEST_F(CPUTest, OpcodeFX1EAddsVxToI) {
    cpu.m_I = 0x300;
    cpu.m_V[5] = 0x10;
    cpu.execute(0xF51E, display, keypad);
    EXPECT_EQ(cpu.m_I, 0x310);
}

TEST_F(CPUTest, OpcodeFX29SetsIToFontCharacterAddress) {
    cpu.m_V[3] = 0x0;
    cpu.execute(0xF329, display, keypad);
    EXPECT_EQ(cpu.m_I, cpu.FONT_START_ADDRESS); // 0 is the first character

    cpu.m_V[3] = 0xF;
    cpu.execute(0xF329, display, keypad);
    EXPECT_EQ(cpu.m_I, cpu.FONT_START_ADDRESS + 0xF * cpu.FONT_CHAR_SIZE); // F is the last character
}

TEST_F(CPUTest, OpcodeFX29OnlyUsesLowNibbleOfVx) {
    // Upper nibble should be ignored, only digits 0-F are valid font characters
    cpu.m_V[4] = 0xAB;
    cpu.execute(0xF429, display, keypad);
    EXPECT_EQ(cpu.m_I, cpu.FONT_START_ADDRESS + 0xB * cpu.FONT_CHAR_SIZE);
}

TEST_F(CPUTest, OpcodeFX33StoresBCDOfVx) {
    cpu.m_I = 0x300;
    cpu.m_V[2] = 156;
    cpu.execute(0xF233, display, keypad);
    EXPECT_EQ(cpu.m_memory[cpu.m_I], 1);
    EXPECT_EQ(cpu.m_memory[cpu.m_I + 1], 5);
    EXPECT_EQ(cpu.m_memory[cpu.m_I + 2], 6);
}

TEST_F(CPUTest, OpcodeFX33HandlesSingleDigitValue) {
    cpu.m_I = 0x300;
    cpu.m_V[2] = 7;
    cpu.execute(0xF233, display, keypad);
    EXPECT_EQ(cpu.m_memory[cpu.m_I], 0);
    EXPECT_EQ(cpu.m_memory[cpu.m_I + 1], 0);
    EXPECT_EQ(cpu.m_memory[cpu.m_I + 2], 7);
}

TEST_F(CPUTest, OpcodeFX55StoresV0ThroughVxInMemoryStartingAtI) {
    cpu.m_I = 0x300;
    cpu.m_V[0] = 0x11;
    cpu.m_V[1] = 0x22;
    cpu.m_V[2] = 0x33;
    cpu.execute(0xF255, display, keypad);
    EXPECT_EQ(cpu.m_memory[0x300], 0x11);
    EXPECT_EQ(cpu.m_memory[0x301], 0x22);
    EXPECT_EQ(cpu.m_memory[0x302], 0x33);
}

TEST_F(CPUTest, OpcodeFX55SetsIToIPlusXPlusOne) {
    cpu.m_I = 0x300;
    cpu.execute(0xF255, display, keypad);
    EXPECT_EQ(cpu.m_I, 0x303);
}

TEST_F(CPUTest, OpcodeFX65FillsV0ThroughVxFromMemoryStartingAtI) {
    cpu.m_I = 0x300;
    set_memory(0x300, {0x11, 0x22, 0x33});
    cpu.execute(0xF265, display, keypad);
    EXPECT_EQ(cpu.m_V[0], 0x11);
    EXPECT_EQ(cpu.m_V[1], 0x22);
    EXPECT_EQ(cpu.m_V[2], 0x33);
}

TEST_F(CPUTest, OpcodeFX65SetsIToIPlusXPlusOne) {
    cpu.m_I = 0x300;
    cpu.execute(0xF265, display, keypad);
    EXPECT_EQ(cpu.m_I, 0x303);
}

TEST_F(CPUTest, OpcodeFX55ThenFX65RoundTripsRegisterValues) {
    cpu.m_I = 0x300;
    cpu.m_V[0] = 0xAA;
    cpu.m_V[1] = 0xBB;
    cpu.m_V[2] = 0xCC;
    cpu.execute(0xF255, display, keypad);

    cpu.m_V[0] = 0;
    cpu.m_V[1] = 0;
    cpu.m_V[2] = 0;
    cpu.m_I = 0x300;
    cpu.execute(0xF265, display, keypad);

    EXPECT_EQ(cpu.m_V[0], 0xAA);
    EXPECT_EQ(cpu.m_V[1], 0xBB);
    EXPECT_EQ(cpu.m_V[2], 0xCC);
}

TEST_F(CPUTest, OpcodeFXNNThrowsOnUnknownSubOpcode) {
    EXPECT_THROW(cpu.execute(0xF000, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, UpdateTimersDecrementsDelayAndSoundTimers) {
    cpu.m_delay_timer = 2;
    cpu.m_sound_timer = 2;

    cpu.updateTimers();

    EXPECT_EQ(cpu.m_delay_timer, 1);
    EXPECT_EQ(cpu.m_sound_timer, 1);
}

TEST_F(CPUTest, UpdateTimersDoNotUnderflowPastZero) {
    cpu.m_delay_timer = 0;
    cpu.m_sound_timer = 0;

    cpu.updateTimers();

    EXPECT_EQ(cpu.m_delay_timer, 0);
    EXPECT_EQ(cpu.m_sound_timer, 0);
}

TEST_F(CPUTest, FetchThrowsWhenOutOfBounds) {
    cpu.m_pc = 4095;
    EXPECT_THROW(cpu.fetch(), std::out_of_range);
}

TEST_F(CPUTest, ExecuteThrowsOnUnknownOpcode) {
    EXPECT_THROW(cpu.execute(0x0000, display, keypad), std::invalid_argument);
    EXPECT_THROW(cpu.execute(0xE000, display, keypad), std::invalid_argument);
}

TEST_F(CPUTest, LoadRomLoadsBytesAtRomStartAddress) {
    const auto rom_path{ std::filesystem::temp_directory_path() / "chip8_test_rom.ch8" };
    {
        // Create a tiny fake ROM file on disk so load_rom reads real binary input
        std::ofstream rom_file(rom_path, std::ios::binary);
        ASSERT_TRUE(rom_file.is_open());
        const std::vector<uint8_t> rom_data{0x12, 0x34, 0xAB, 0xCD};
        rom_file.write(reinterpret_cast<const char*>(rom_data.data()), static_cast<std::streamsize>(rom_data.size()));
    }

    cpu.load_rom(rom_path.string());

    // ROM bytes should be copied into RAM starting at 0x200
    EXPECT_EQ(cpu.m_memory[cpu.ROM_START_ADDRESS], 0x12);
    EXPECT_EQ(cpu.m_memory[cpu.ROM_START_ADDRESS + 1], 0x34);
    EXPECT_EQ(cpu.m_memory[cpu.ROM_START_ADDRESS + 2], 0xAB);
    EXPECT_EQ(cpu.m_memory[cpu.ROM_START_ADDRESS + 3], 0xCD);

    std::filesystem::remove(rom_path);
}

TEST_F(CPUTest, LoadRomThrowsWhenFileDoesNotExist) {
    EXPECT_THROW(cpu.load_rom("/tmp/does_not_exist_chip8_rom.ch8"), std::runtime_error);
}

TEST_F(CPUTest, LoadRomThrowsWhenRomTooLarge) {
    const auto rom_path{ std::filesystem::temp_directory_path() / "chip8_test_large_rom.ch8" };
    {
        std::ofstream rom_file(rom_path, std::ios::binary);
        ASSERT_TRUE(rom_file.is_open());
        // Make a ROM 1 byte larger than available program memory to trigger size check.
        std::vector<uint8_t> rom_data(cpu.MEMORY_SIZE - cpu.ROM_START_ADDRESS + 1, 0xFF);
        rom_file.write(reinterpret_cast<const char*>(rom_data.data()), static_cast<std::streamsize>(rom_data.size()));
    }

    EXPECT_THROW(cpu.load_rom(rom_path.string()), std::runtime_error);

    std::filesystem::remove(rom_path);
}

// The following tests are ported from the "flags" test ROM
// see: https://github.com/Timendus/chip8-test-suite/blob/main/src/tests/4-flags.8o
TEST_F(CPUTest, FlagsRomOrNoCarry) {
    cpu.m_V[3] = 15;
    cpu.m_V[0xF] = 20;
    cpu.execute(0x83F1, display, keypad); // v3 |= vF -> 15 | 20 = 31

    cpu.m_V[0xF] = 0;
    cpu.m_V[2] = 50;
    cpu.m_V[1] = 15;
    cpu.execute(0x8211, display, keypad); // v2 |= v1 -> 50 | 15 = 63

    EXPECT_EQ(cpu.m_V[2], 63);
    EXPECT_EQ(cpu.m_V[0xF], 0); // VF must not be clobbered by an OR that doesn't target VF
    EXPECT_EQ(cpu.m_V[3], 31);
}

TEST_F(CPUTest, FlagsRomAndNoCarry) {
    cpu.m_V[3] = 15;
    cpu.m_V[0xF] = 20;
    cpu.execute(0x83F2, display, keypad); // v3 &= vF -> 15 & 20 = 4

    cpu.m_V[0xF] = 0;
    cpu.m_V[2] = 50;
    cpu.m_V[1] = 15;
    cpu.execute(0x8212, display, keypad); // v2 &= v1 -> 50 & 15 = 2

    EXPECT_EQ(cpu.m_V[2], 2);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 4);
}

TEST_F(CPUTest, FlagsRomXorNoCarry) {
    cpu.m_V[3] = 15;
    cpu.m_V[0xF] = 20;
    cpu.execute(0x83F3, display, keypad); // v3 ^= vF -> 15 ^ 20 = 27

    cpu.m_V[0xF] = 0;
    cpu.m_V[2] = 50;
    cpu.m_V[1] = 15;
    cpu.execute(0x8213, display, keypad); // v2 ^= v1 -> 50 ^ 15 = 61

    EXPECT_EQ(cpu.m_V[2], 61);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 27);
}

TEST_F(CPUTest, FlagsRomAddNoOverflow) {
    cpu.m_V[0xF] = 20;
    cpu.m_V[1] = 15;
    cpu.execute(0x8F14, display, keypad); // vF += v1 -> sum 35, but VF must end up holding the carry flag (0)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 15;
    cpu.m_V[0xF] = 20;
    cpu.execute(0x83F4, display, keypad); // v3 += vF -> 15 + 20 = 35

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 50;
    cpu.m_V[1] = 15;
    cpu.execute(0x8214, display, keypad); // v2 += v1 -> 50 + 15 = 65

    EXPECT_EQ(cpu.m_V[2], 65);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 35);
    EXPECT_EQ(cpu.m_V[4], 0);
}

TEST_F(CPUTest, FlagsRomSubVxMinusVyNoBorrow) {
    cpu.m_V[0xF] = 20;
    cpu.m_V[1] = 15;
    cpu.execute(0x8F15, display, keypad); // vF -= v1 -> 20 - 15 = 5, but VF must end up holding the no-borrow flag (1)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 20;
    cpu.m_V[0xF] = 15;
    cpu.execute(0x83F5, display, keypad); // v3 -= vF -> 20 - 15 = 5

    // N - N (equal operands) must not report a borrow
    cpu.m_V[5] = 10;
    cpu.m_V[0xF] = 10;
    cpu.execute(0x85F5, display, keypad); // v5 -= vF -> 10 - 10 = 0, flag = 1 (no borrow)
    EXPECT_EQ(cpu.m_V[0xF], 1);
    cpu.m_V[5] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 50;
    cpu.m_V[1] = 15;
    cpu.execute(0x8215, display, keypad); // v2 -= v1 -> 50 - 15 = 35

    EXPECT_EQ(cpu.m_V[2], 35);
    EXPECT_EQ(cpu.m_V[0xF], 1);
    EXPECT_EQ(cpu.m_V[3], 5);
    EXPECT_EQ(cpu.m_V[4], 1);
    EXPECT_EQ(cpu.m_V[5], 1);
}

TEST_F(CPUTest, FlagsRomShrNoLsb) {
    cpu.m_V[0xF] = 60; // LSB is 0
    cpu.execute(0x8FF6, display, keypad); // vF >>= vF -> 60 >> 1 = 30, but VF must end up holding the shifted-out bit (0)
    cpu.m_V[3] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 60;
    cpu.execute(0x8226, display, keypad); // v2 >>= v2 -> 60 >> 1 = 30

    EXPECT_EQ(cpu.m_V[2], 30);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 0);
}

TEST_F(CPUTest, FlagsRomSubnVyMinusVxNoBorrow) {
    cpu.m_V[0xF] = 10;
    cpu.m_V[1] = 15;
    cpu.execute(0x8F17, display, keypad); // vF =- v1 -> 15 - 10 = 5, but VF must end up holding the no-borrow flag (1)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 15;
    cpu.m_V[0xF] = 20;
    cpu.execute(0x83F7, display, keypad); // v3 =- vF -> 20 - 15 = 5

    // N - N (equal operands) must not report a borrow
    cpu.m_V[5] = 10;
    cpu.m_V[0xF] = 10;
    cpu.execute(0x85F7, display, keypad); // v5 =- vF -> 10 - 10 = 0, flag = 1 (no borrow)
    EXPECT_EQ(cpu.m_V[0xF], 1);
    cpu.m_V[5] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 15;
    cpu.m_V[1] = 50;
    cpu.execute(0x8217, display, keypad); // v2 =- v1 -> 50 - 15 = 35

    EXPECT_EQ(cpu.m_V[2], 35);
    EXPECT_EQ(cpu.m_V[0xF], 1);
    EXPECT_EQ(cpu.m_V[3], 5);
    EXPECT_EQ(cpu.m_V[4], 1);
    EXPECT_EQ(cpu.m_V[5], 1);
}

TEST_F(CPUTest, FlagsRomShlNoMsb) {
    cpu.m_V[0xF] = 50; // MSB is 0
    cpu.execute(0x8FFE, display, keypad); // vF <<= vF -> 50 << 1 = 100, but VF must end up holding the shifted-out bit (0)
    cpu.m_V[3] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 50;
    cpu.execute(0x822E, display, keypad); // v2 <<= v2 -> 50 << 1 = 100

    EXPECT_EQ(cpu.m_V[2], 100);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 0);
}

TEST_F(CPUTest, FlagsRomAddWithOverflow) {
    cpu.m_V[0xF] = 200;
    cpu.m_V[1] = 100;
    cpu.execute(0x8F14, display, keypad); // vF += v1 -> 300 wraps to 44, but VF must end up holding the carry flag (1)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 100;
    cpu.m_V[0xF] = 200;
    cpu.execute(0x83F4, display, keypad); // v3 += vF -> 100 + 200 = 300 wraps to 44

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 200;
    cpu.m_V[1] = 100;
    cpu.execute(0x8214, display, keypad); // v2 += v1 -> 200 + 100 = 300 wraps to 44

    EXPECT_EQ(cpu.m_V[2], 44);
    EXPECT_EQ(cpu.m_V[0xF], 1);
    EXPECT_EQ(cpu.m_V[3], 44);
    EXPECT_EQ(cpu.m_V[4], 1);
}

TEST_F(CPUTest, FlagsRomSubVxMinusVyWithBorrow) {
    cpu.m_V[0xF] = 95;
    cpu.m_V[1] = 100;
    cpu.execute(0x8F15, display, keypad); // vF -= v1 -> 95 - 100 wraps to 251, but VF must end up holding the borrow flag (0)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 95;
    cpu.m_V[0xF] = 100;
    cpu.execute(0x83F5, display, keypad); // v3 -= vF -> 95 - 100 wraps to 251

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 95;
    cpu.m_V[1] = 100;
    cpu.execute(0x8215, display, keypad); // v2 -= v1 -> 95 - 100 wraps to 251

    EXPECT_EQ(cpu.m_V[2], 251);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 251);
    EXPECT_EQ(cpu.m_V[4], 0);
}

TEST_F(CPUTest, FlagsRomShrWithLsb) {
    cpu.m_V[0xF] = 61; // LSB is 1
    cpu.execute(0x8FF6, display, keypad); // vF >>= vF -> 61 >> 1 = 30, but VF must end up holding the shifted-out bit (1)
    cpu.m_V[3] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 61;
    cpu.execute(0x8226, display, keypad); // v2 >>= v2 -> 61 >> 1 = 30

    EXPECT_EQ(cpu.m_V[2], 30);
    EXPECT_EQ(cpu.m_V[0xF], 1);
    EXPECT_EQ(cpu.m_V[3], 1);
}

TEST_F(CPUTest, FlagsRomSubnVyMinusVxWithBorrow) {
    cpu.m_V[0xF] = 105;
    cpu.m_V[1] = 100;
    cpu.execute(0x8F17, display, keypad); // vF =- v1 -> 100 - 105 wraps to 251, but VF must end up holding the borrow flag (0)
    cpu.m_V[4] = cpu.m_V[0xF];

    cpu.m_V[3] = 105;
    cpu.m_V[0xF] = 100;
    cpu.execute(0x83F7, display, keypad); // v3 =- vF -> 100 - 105 wraps to 251

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 105;
    cpu.m_V[1] = 100;
    cpu.execute(0x8217, display, keypad); // v2 =- v1 -> 100 - 105 wraps to 251

    EXPECT_EQ(cpu.m_V[2], 251);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    EXPECT_EQ(cpu.m_V[3], 251);
    EXPECT_EQ(cpu.m_V[4], 0);
}

TEST_F(CPUTest, FlagsRomShlWithMsb) {
    cpu.m_V[0xF] = 188; // MSB is 1
    cpu.execute(0x8FFE, display, keypad); // vF <<= vF -> 188 << 1 = 376 wraps to 120, but VF must end up holding the shifted-out bit (1)
    cpu.m_V[3] = cpu.m_V[0xF];

    cpu.m_V[0xF] = 0xAA;
    cpu.m_V[2] = 188;
    cpu.execute(0x822E, display, keypad); // v2 <<= v2 -> 188 << 1 = 376 wraps to 120

    EXPECT_EQ(cpu.m_V[2], 120);
    EXPECT_EQ(cpu.m_V[0xF], 1);
    EXPECT_EQ(cpu.m_V[3], 1);
}

TEST_F(CPUTest, FlagsRomAddToIndexUsingRegularRegister) {
    cpu.m_I = 0x300;
    cpu.m_V[1] = 16;
    cpu.execute(0xF11E, display, keypad); // i += v1

    cpu.m_V[0] = 0xAA;
    cpu.execute(0xF055, display, keypad); // save v0 (stores memory[I] = v0)

    cpu.m_I = 0x310;
    cpu.m_V[0] = 0;
    cpu.execute(0xF065, display, keypad); // load v0 (loads v0 = memory[I])

    EXPECT_EQ(cpu.m_V[0], 0xAA);
}

TEST_F(CPUTest, FlagsRomAddToIndexUsingVFAsSource) {
    cpu.m_I = 0x300;
    cpu.m_V[0xF] = 16;
    cpu.execute(0xFF1E, display, keypad); // i += vF

    cpu.m_V[0] = 0x55;
    cpu.execute(0xF055, display, keypad); // save v0 (stores memory[I] = v0)

    cpu.m_I = 0x310;
    cpu.m_V[0] = 0;
    cpu.execute(0xF065, display, keypad); // load v0 (loads v0 = memory[I])

    EXPECT_EQ(cpu.m_V[0], 0x55);
}
