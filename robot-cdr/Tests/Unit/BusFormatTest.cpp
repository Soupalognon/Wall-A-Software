#include <gtest/gtest.h>
#include <cstring>
#include "Services/BusFormat.h"

// BusFormat.cpp format: "TEL ODO %.2f %.2f %.2f\n"
TEST(BusFormat, telOdoExactFormat) {
    const char* s = BusFormat::telOdo(1.23f, 0.45f, 90.0f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s), "TEL ODO 1.23 0.45 90.00\n");
}

TEST(BusFormat, telOdoNewlineTerminator) {
    const char* s = BusFormat::telOdo(0.0f, 0.0f, 0.0f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

// BusFormat.cpp format: "ALT PROXIMITY %.2f\n"
TEST(BusFormat, altProximityStartsWithALT) {
    const char* s = BusFormat::altProximity(0.12f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s).substr(0, 4), "ALT ");
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

TEST(BusFormat, altProximityExactFormat) {
    const char* s = BusFormat::altProximity(1.50f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s), "ALT PROXIMITY 1.50\n");
}

// BusFormat.cpp format: "LOG INFO %s\n"
TEST(BusFormat, logInfoExactFormat) {
    const char* s = BusFormat::logInfo("SystemInit OK");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s), "LOG INFO SystemInit OK\n");
}

TEST(BusFormat, logInfoNewlineTerminator) {
    const char* s = BusFormat::logInfo("test");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

// BusFormat.cpp format: "HLT TEMP %.2f\n"
TEST(BusFormat, hltTempStartsWithHLT) {
    const char* s = BusFormat::hltTemp(36.5f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s).substr(0, 4), "HLT ");
    EXPECT_EQ(s[strlen(s) - 1], '\n');
}

TEST(BusFormat, hltTempExactFormat) {
    const char* s = BusFormat::hltTemp(36.5f);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::string(s), "HLT TEMP 36.50\n");
}

// All methods must return non-null
TEST(BusFormat, returnsNonNull) {
    EXPECT_NE(BusFormat::telOdo(0, 0, 0), nullptr);
    EXPECT_NE(BusFormat::altProximity(0), nullptr);
    EXPECT_NE(BusFormat::logInfo(""), nullptr);
    EXPECT_NE(BusFormat::hltTemp(0), nullptr);
}
