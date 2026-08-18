#include <gtest/gtest.h>
#include "../src/CPU.h"
#include "../src/Display.h"
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
    using CPU::fetch;
    using CPU::execute;
    using CPU::MEMORY_SIZE;
    using CPU::ROM_START_ADDRESS;
    using CPU::STACK_DEPTH;
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

    void set_memory(uint16_t address, const std::vector<uint8_t>& data) {
        for (size_t i{0}; i < data.size(); ++i) {
            cpu.m_memory[address + i] = data[i];
        }
    }
};

TEST_F(CPUTest, FontsetLoadedAtInitialization) {
    // Fontset starts at 0x50
    // First char '0': 0xF0, 0x90, 0x90, 0x90, 0xF0
    EXPECT_EQ(cpu.m_memory[0x50], 0xF0);
    EXPECT_EQ(cpu.m_memory[0x51], 0x90);
    EXPECT_EQ(cpu.m_memory[0x52], 0x90);
    EXPECT_EQ(cpu.m_memory[0x53], 0x90);
    EXPECT_EQ(cpu.m_memory[0x54], 0xF0);
    
    // Last char 'F': 0xF0, 0x80, 0xF0, 0x80, 0x80 (at 0x50 + 15*5)
    EXPECT_EQ(cpu.m_memory[0x50 + 15*5], 0xF0);
    EXPECT_EQ(cpu.m_memory[0x50 + 15*5+4], 0x80);
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
    
    cpu.execute(0x00E0, display);
    
    EXPECT_TRUE(display.is_cleared());
}

TEST_F(CPUTest, Opcode1NNNJumpsToAddress) {
    cpu.execute(0x1250, display);
    EXPECT_EQ(cpu.m_pc, 0x250);
}

TEST_F(CPUTest, Opcode00EEReturnsFromSubroutine) {
    cpu.m_stack.push(0x350);

    cpu.execute(0x00EE, display);

    EXPECT_EQ(cpu.m_pc, 0x350);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode00EEThrowsWhenStackIsEmpty) {
    EXPECT_TRUE(cpu.m_stack.empty());
    EXPECT_THROW(cpu.execute(0x00EE, display), std::out_of_range);
}

TEST_F(CPUTest, Opcode2NNNPushesPCAndJumpsToAddress) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2300, display);

    EXPECT_EQ(cpu.m_pc, 0x300);
    ASSERT_FALSE(cpu.m_stack.empty());
    EXPECT_EQ(cpu.m_stack.top(), start_pc);
}

TEST_F(CPUTest, Opcode2NNNThrowsOnInvalidAddress) {
    EXPECT_THROW(cpu.execute(0x2100, display), std::invalid_argument);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode2NNNThrowsOnStackOverflow) {
    // Fill the stack to STACK_DEPTH
    for (size_t i{0}; i < cpu.STACK_DEPTH; ++i) {
        cpu.execute(0x2300, display);
    }
    EXPECT_EQ(cpu.m_stack.size(), cpu.STACK_DEPTH);

    // One more call should overflow rather than grow the stack unbounded
    EXPECT_THROW(cpu.execute(0x2300, display), std::overflow_error);
    EXPECT_EQ(cpu.m_stack.size(), cpu.STACK_DEPTH);
}

TEST_F(CPUTest, Opcode2NNNThenOpcode00EERoundTripsToCallSite) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2400, display); // Call 0x400
    EXPECT_EQ(cpu.m_pc, 0x400);

    cpu.execute(0x00EE, display); // Return
    EXPECT_EQ(cpu.m_pc, start_pc);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode2NNNSupportsNestedCalls) {
    uint16_t start_pc{cpu.m_pc};

    cpu.execute(0x2300, display); // Call 0x300, pushes start_pc
    uint16_t after_first_call{cpu.m_pc};
    cpu.execute(0x2400, display); // Call 0x400, pushes after_first_call
    EXPECT_EQ(cpu.m_pc, 0x400);
    EXPECT_EQ(cpu.m_stack.size(), 2);

    cpu.execute(0x00EE, display); // Return to after_first_call
    EXPECT_EQ(cpu.m_pc, after_first_call);

    cpu.execute(0x00EE, display); // Return to start_pc
    EXPECT_EQ(cpu.m_pc, start_pc);
    EXPECT_TRUE(cpu.m_stack.empty());
}

TEST_F(CPUTest, Opcode3XNNSkipsWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x31;

    cpu.execute(0x3331, display);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode3XNNDoesNotSkipWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[3] = 0x32;

    cpu.execute(0x3331, display);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode4XNNDoesNotSkipWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[4] = 0x41;

    cpu.execute(0x4441, display);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode4XNNSkipsWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[4] = 0x42;

    cpu.execute(0x4441, display);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode5XY0SkipsWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x51;
    cpu.m_V[2] = 0x51;

    cpu.execute(0x5120, display);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode5XY0DoesNotSkipWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x51;
    cpu.m_V[2] = 0x52;

    cpu.execute(0x5120, display);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode5XY0ThrowsOnInvalidSubOpcode) {
    EXPECT_THROW(cpu.execute(0x5121, display), std::invalid_argument);
}

TEST_F(CPUTest, Opcode6XNNSetsRegister) {
    cpu.execute(0x62FF, display);
    EXPECT_EQ(cpu.m_V[2], 0xFF);
}

TEST_F(CPUTest, Opcode7XNNAddsToRegister) {
    cpu.m_V[3] = 0x10;
    cpu.execute(0x7305, display);
    EXPECT_EQ(cpu.m_V[3], 0x15);
    
    // Test wrap around
    cpu.m_V[3] = 0xFE;
    cpu.execute(0x7304, display);
    EXPECT_EQ(cpu.m_V[3], 0x02);
}

TEST_F(CPUTest, Opcode8XY0SetsVxToVy) {
    cpu.m_V[1] = 0x11;
    cpu.m_V[2] = 0x22;

    cpu.execute(0x8120, display);

    EXPECT_EQ(cpu.m_V[1], 0x22);
}

TEST_F(CPUTest, Opcode8XY1SetsVxToVxORVy) {
    cpu.m_V[1] = 0b1010;
    cpu.m_V[2] = 0b0101;

    cpu.execute(0x8121, display);

    EXPECT_EQ(cpu.m_V[1], 0b1111);
}

TEST_F(CPUTest, Opcode8XY2SetsVxToVxANDVy) {
    cpu.m_V[1] = 0b1100;
    cpu.m_V[2] = 0b1010;

    cpu.execute(0x8122, display);

    EXPECT_EQ(cpu.m_V[1], 0b1000);
}

TEST_F(CPUTest, Opcode8XY3SetsVxToVxXORVy) {
    cpu.m_V[1] = 0b1100;
    cpu.m_V[2] = 0b1010;

    cpu.execute(0x8123, display);

    EXPECT_EQ(cpu.m_V[1], 0b0110);
}

TEST_F(CPUTest, Opcode8XY4AddsVyToVxWithoutCarry) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8124, display);

    EXPECT_EQ(cpu.m_V[1], 0x15);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XY4AddsVyToVxWithCarry) {
    cpu.m_V[1] = 0xFE;
    cpu.m_V[2] = 0x04;

    cpu.execute(0x8124, display);

    EXPECT_EQ(cpu.m_V[1], 0x02); // wraps around mod 256
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XY5SubtractsVyFromVxWithoutBorrow) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8125, display);

    EXPECT_EQ(cpu.m_V[1], 0x0B);
    EXPECT_EQ(cpu.m_V[0xF], 1); // VX >= VY, no borrow
}

TEST_F(CPUTest, Opcode8XY5SubtractsVyFromVxWithBorrow) {
    cpu.m_V[1] = 0x05;
    cpu.m_V[2] = 0x10;

    cpu.execute(0x8125, display);

    EXPECT_EQ(cpu.m_V[1], 0xF5); // wraps around
    EXPECT_EQ(cpu.m_V[0xF], 0); // VX < VY, borrow occurred
}

TEST_F(CPUTest, Opcode8XY6ShiftsVyRightIntoVxAndSetsVFToShiftedOutBit) {
    cpu.m_V[2] = 0b00000011; // least significant bit is 1

    cpu.execute(0x8126, display);

    EXPECT_EQ(cpu.m_V[1], 0b00000001);
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XY6ClearsVFWhenShiftedOutBitIsZero) {
    cpu.m_V[2] = 0b00000010; // least significant bit is 0

    cpu.execute(0x8126, display);

    EXPECT_EQ(cpu.m_V[1], 0b00000001);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XY7SetsVxToVyMinusVxWithoutBorrow) {
    cpu.m_V[1] = 0x05;
    cpu.m_V[2] = 0x10;

    cpu.execute(0x8127, display);

    EXPECT_EQ(cpu.m_V[1], 0x0B);
    EXPECT_EQ(cpu.m_V[0xF], 1); // VY >= VX, no borrow
}

TEST_F(CPUTest, Opcode8XY7SetsVxToVyMinusVxWithBorrow) {
    cpu.m_V[1] = 0x10;
    cpu.m_V[2] = 0x05;

    cpu.execute(0x8127, display);

    EXPECT_EQ(cpu.m_V[1], 0xF5); // wraps around
    EXPECT_EQ(cpu.m_V[0xF], 0); // VY < VX, borrow occurred
}

TEST_F(CPUTest, Opcode8XYEShiftsVyLeftIntoVxAndSetsVFToShiftedOutBit) {
    cpu.m_V[2] = 0b11000000; // most significant bit is 1

    cpu.execute(0x812E, display);

    EXPECT_EQ(cpu.m_V[1], 0b10000000);
    EXPECT_EQ(cpu.m_V[0xF], 1);
}

TEST_F(CPUTest, Opcode8XYEClearsVFWhenShiftedOutBitIsZero) {
    cpu.m_V[2] = 0b01000000; // most significant bit is 0

    cpu.execute(0x812E, display);

    EXPECT_EQ(cpu.m_V[1], 0b10000000);
    EXPECT_EQ(cpu.m_V[0xF], 0);
}

TEST_F(CPUTest, Opcode8XYNThrowsOnUnknownSubOpcode) {
    EXPECT_THROW(cpu.execute(0x8128, display), std::invalid_argument);
    EXPECT_THROW(cpu.execute(0x8129, display), std::invalid_argument);
}

TEST_F(CPUTest, Opcode9XY0SkipsWhenNotEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x91;
    cpu.m_V[2] = 0x92;

    cpu.execute(0x9120, display);

    EXPECT_EQ(cpu.m_pc, start_pc + 2);
}

TEST_F(CPUTest, Opcode9XY0DoesNotSkipWhenEqual) {
    uint16_t start_pc{cpu.m_pc};
    cpu.m_V[1] = 0x91;
    cpu.m_V[2] = 0x91;

    cpu.execute(0x9120, display);

    EXPECT_EQ(cpu.m_pc, start_pc);
}

TEST_F(CPUTest, Opcode9XY0ThrowsOnInvalidSubOpcode) {
    EXPECT_THROW(cpu.execute(0x9121, display), std::invalid_argument);
}

TEST_F(CPUTest, OpcodeANNNSetsIRegister) {
    cpu.execute(0xAABC, display);
    EXPECT_EQ(cpu.m_I, 0xABC);
}

TEST_F(CPUTest, OpcodeBNNNJumpsToAddressPlusV0) {
    cpu.m_V[0] = 0x10;
    cpu.execute(0xB300, display);
    EXPECT_EQ(cpu.m_pc, 0x310);
}

TEST_F(CPUTest, OpcodeDXYNDrawsSpriteAndSetsCollision) {
    // Set I to a simple sprite (single pixel at top left)
    cpu.m_I = 0x300;
    set_memory(0x300, {0x80}); // 10000000
    
    cpu.m_V[0] = 0;
    cpu.m_V[1] = 0;
    
    // Draw it
    cpu.execute(0xD011, display);
    EXPECT_EQ(display.get_pixel_at(0, 0), 1);
    EXPECT_EQ(cpu.m_V[0xF], 0);
    
    // Draw it again at same position, should collision and clear
    cpu.execute(0xD011, display);
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

    cpu.execute(0xD012, display);
    
    // Only the top-left pixel of the sprite at (63, 31) should be drawn
    EXPECT_EQ(display.get_pixel_at(Display::WIDTH - 1, Display::HEIGHT - 1), 1);
    
    // Test Case 2: Coordinate wrapping
    // The STARTING position wraps
    // Set position to (66, 34), which should wrap to (2, 2)
    display.clear();
    cpu.m_V[0] = Display::WIDTH + 2;
    cpu.m_V[1] = Display::HEIGHT + 2;
    
    cpu.execute(0xD012, display);
    
    // Check that sprite was drawn at the wrapped coordinates (2, 2)
    EXPECT_EQ(display.get_pixel_at(2, 2), 1);
    EXPECT_EQ(display.get_pixel_at(3, 2), 1);
    EXPECT_EQ(display.get_pixel_at(2, 3), 1);
    EXPECT_EQ(display.get_pixel_at(3, 3), 1);
}

TEST_F(CPUTest, FetchThrowsWhenOutOfBounds) {
    cpu.m_pc = 4095;
    EXPECT_THROW(cpu.fetch(), std::out_of_range);
}

TEST_F(CPUTest, ExecuteThrowsOnUnknownOpcode) {
    EXPECT_THROW(cpu.execute(0x0000, display), std::invalid_argument);
    EXPECT_THROW(cpu.execute(0xE000, display), std::invalid_argument);
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
