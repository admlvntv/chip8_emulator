#include <gtest/gtest.h>
#include "../src/CPU.h"
#include "../src/Display.h"
#include <vector>

class CPUForTesting : public CPU {
public:
    using CPU::m_memory;
    using CPU::m_V;
    using CPU::m_I;
    using CPU::m_pc;
    using CPU::m_stack;
    using CPU::m_sp;
    using CPU::fetch;
    using CPU::execute;
    using CPU::MEMORY_SIZE;
    using CPU::ROM_START_ADDRESS;
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

TEST_F(CPUTest, OpcodeANNNSetsIRegister) {
    cpu.execute(0xAABC, display);
    EXPECT_EQ(cpu.m_I, 0xABC);
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
