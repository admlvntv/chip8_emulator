#include <gtest/gtest.h>
#include "../src/Display.h"

class DummyDisplay : public Display {
public:
    void render() const override {}
};

TEST(DisplayTest, InitializationClearsScreen) {
    DummyDisplay display;
    for (int y{0}; y < Display::HEIGHT; ++y) {
        for (int x{0}; x < Display::WIDTH; ++x) {
            EXPECT_EQ(display.get_pixel(x, y), 0);
        }
    }
}

TEST(DisplayTest, ClearResetsAllPixels) {
    DummyDisplay display;
    display.write_pixel(10, 10);
    display.write_pixel(20, 20);
    
    display.clear();
    
    for (int y{0}; y < Display::HEIGHT; ++y) {
        for (int x{0}; x < Display::WIDTH; ++x) {
            EXPECT_EQ(display.get_pixel(x, y), 0);
        }
    }
}

TEST(DisplayTest, WritePixelXorsValueAndReturnsCollision) {
    DummyDisplay display;
    
    // Set pixel (0 -> 1)
    bool collision{ display.write_pixel(5, 5) };
    EXPECT_FALSE(collision);
    EXPECT_EQ(display.get_pixel(5, 5), 1);
    
    // Flip pixel back (1 -> 0)
    collision = display.write_pixel(5, 5);
    EXPECT_TRUE(collision);
    EXPECT_EQ(display.get_pixel(5, 5), 0);
}

TEST(DisplayTest, WritePixelOutOfBoundsReturnsFalse) {
    DummyDisplay display;
    EXPECT_FALSE(display.write_pixel(-1, 0));
    EXPECT_FALSE(display.write_pixel(Display::WIDTH, 0));
    EXPECT_FALSE(display.write_pixel(0, -1));
    EXPECT_FALSE(display.write_pixel(0, Display::HEIGHT));
}

TEST(DisplayTest, GetPixelOutOfBoundsReturnsZero) {
    DummyDisplay display;
    EXPECT_EQ(display.get_pixel(-1, 0), 0);
    EXPECT_EQ(display.get_pixel(Display::WIDTH, 0), 0);
}
