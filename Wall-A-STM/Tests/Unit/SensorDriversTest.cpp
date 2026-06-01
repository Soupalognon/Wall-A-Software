#include <gtest/gtest.h>
#include "Stubs/HalStub.h"
#include "Mocks/FakeAdcHAL.h"
#include "Mocks/FakeInputCaptureHAL.h"
#include "Services/B5WLB2101.h"
#include "Services/MotorCurrentSense.h"
#include "Services/InternalTemperature.h"
#include "Services/ProximeterPololu5472.h"

// Au quart d'échelle « pile au milieu » : raw=2048 → 2048/4096*3.3 = 1.65 V.
// Cette valeur tombe sur des points propres dans les trois conversions.
static constexpr uint16_t RAW_MID = 2048;  // → 1.65 V

// ── B5WLB2101 — tension brute (V = raw/4096 * 3.3) ───────────────────────────

TEST(B5WLB2101Test, Read_ConvertsRawToVoltage) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;
    B5WLB2101 s{adc, 20, "PROX_CH1", 3.0f};
    EXPECT_NEAR(1.65f, s.read(), 1e-3f);
}

TEST(B5WLB2101Test, Read_Extremes) {
    FakeAdcHAL adc;
    B5WLB2101 s{adc, 20, "PROX_CH1", 3.0f};
    adc.raw = 0;     EXPECT_NEAR(0.0f, s.read(), 1e-3f);
    adc.raw = 4096;  EXPECT_NEAR(3.3f, s.read(), 1e-3f);
}

TEST(B5WLB2101Test, IdAndName) {
    FakeAdcHAL adc;
    B5WLB2101 s{adc, 21, "PROX_CH2", 3.0f};
    EXPECT_EQ(21u, s.id());
    EXPECT_STREQ("PROX_CH2", s.name());
}

TEST(B5WLB2101Test, Alarm_Instant) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // 1.65 V
    B5WLB2101 over{adc, 20, "PROX", 1.0f};
    over.read();
    EXPECT_TRUE(over.isAlarm());

    B5WLB2101 under{adc, 20, "PROX", 2.0f};
    under.read();
    EXPECT_FALSE(under.isAlarm());
}

// Délégation vers l'IAdcHAL injecté (contrat ISensor ↔ driver)
TEST(B5WLB2101Test, DelegatesToAdcHal) {
    FakeAdcHAL adc;
    B5WLB2101 s{adc, 5, "PROX", 1.0f};   // id == SensorType → doneFlag bit 5

    s.bind();
    EXPECT_EQ(1, adc.bindCalls);
    EXPECT_EQ(1u << 5, adc.flag);        // bind() pousse 1<<id dans l'ADC

    s.trigger();
    EXPECT_EQ(1, adc.startCalls);

    EXPECT_EQ(1u << 5, s.doneFlag());
    EXPECT_FALSE(s.isActive());
}

// ── B5WLB2101 — alarme temporelle (periodWindowMs) ───────────────────────────

TEST(B5WLB2101Test, Period_NoAlarm_BeforeDurationElapsed) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // 1.65 V > 1.0 seuil
    B5WLB2101 s{adc, 20, "PROX", 1.0f, 1000};
    setMockTick(0);   s.read();
    setMockTick(999);
    EXPECT_FALSE(s.isAlarm());
}

TEST(B5WLB2101Test, Period_Alarm_AfterDurationElapsed) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;
    B5WLB2101 s{adc, 20, "PROX", 1.0f, 1000};
    setMockTick(0);    s.read();  // rising edge à t=0
    setMockTick(500);  s.read();
    setMockTick(1000);
    EXPECT_TRUE(s.isAlarm());
}

TEST(B5WLB2101Test, Period_NoAlarm_AfterValueDrops) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;
    B5WLB2101 s{adc, 20, "PROX", 1.0f, 1000};
    setMockTick(0);    s.read();   // rising edge
    setMockTick(500);
    adc.raw = 0;       s.read();   // repasse sous le seuil → reset
    setMockTick(1500);
    EXPECT_FALSE(s.isAlarm());
}

// ── MotorCurrentSense — courant moteur ───────────────────────────────────────

TEST(MotorCurrentSenseTest, Read_Primary) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // 1.65 V → I = 1650 / 0.000212 / 3090 ≈ 2518.78 mA
    MotorCurrentSense s{adc, MotorCurrentSense::MotorType::PRIMARY, 7, "CUR_PRI_L", 3000.0f};
    EXPECT_NEAR(2518.78f, s.read(), 0.5f);
}

TEST(MotorCurrentSenseTest, Read_Secondary) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // 1.65 V → I = 1.65 / 0.5 = 3.3 mA
    MotorCurrentSense s{adc, MotorCurrentSense::MotorType::SECONDARY, 9, "CUR_SEC_L", 5.0f};
    EXPECT_NEAR(3.3f, s.read(), 1e-3f);
}

TEST(MotorCurrentSenseTest, Alarm_Primary) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // ≈2518.78 mA
    MotorCurrentSense over{adc, MotorCurrentSense::MotorType::PRIMARY, 7, "CUR", 2000.0f};
    over.read();
    EXPECT_TRUE(over.isAlarm());

    MotorCurrentSense under{adc, MotorCurrentSense::MotorType::PRIMARY, 7, "CUR", 3000.0f};
    under.read();
    EXPECT_FALSE(under.isAlarm());
}

TEST(MotorCurrentSenseTest, IdAndName) {
    FakeAdcHAL adc;
    MotorCurrentSense s{adc, MotorCurrentSense::MotorType::SECONDARY, 10, "CUR_SEC_R", 5.0f};
    EXPECT_EQ(10u, s.id());
    EXPECT_STREQ("CUR_SEC_R", s.name());
}

// ── InternalTemperature — NTC Steinhart–Hart ─────────────────────────────────

TEST(InternalTemperatureTest, Read_25CAtMidScale) {
    // À 1.65 V, R_ntc == R_ref → terme ln nul → T = TEMPERATURE_REFERENCE (25 °C)
    FakeAdcHAL adc;
    adc.raw = RAW_MID;
    InternalTemperature s{adc, 1, "TEMP_PRI", 60.0f};
    EXPECT_NEAR(25.0f, s.read(), 1e-2f);
}

TEST(InternalTemperatureTest, Alarm_Instant) {
    FakeAdcHAL adc;
    adc.raw = RAW_MID;  // 25 °C
    InternalTemperature over{adc, 1, "TEMP", 20.0f};
    over.read();
    EXPECT_TRUE(over.isAlarm());

    InternalTemperature under{adc, 1, "TEMP", 30.0f};
    under.read();
    EXPECT_FALSE(under.isAlarm());
}

TEST(InternalTemperatureTest, IdAndName) {
    FakeAdcHAL adc;
    InternalTemperature s{adc, 3, "TEMP_PWR", 60.0f};
    EXPECT_EQ(3u, s.id());
    EXPECT_STREQ("TEMP_PWR", s.name());
}

// ── ProximeterPololu5472 — largeur d'impulsion (µs) ──────────────────────────

TEST(ProximeterPololu5472Test, Read_ReturnsPulseWidth) {
    FakeInputCaptureHAL ic;
    ic.pulse = 1234;
    ProximeterPololu5472 s{ic, 30, "POL_CH1", 2000.0f};
    EXPECT_NEAR(1234.0f, s.read(), 1e-3f);
}

TEST(ProximeterPololu5472Test, IdAndName) {
    FakeInputCaptureHAL ic;
    ProximeterPololu5472 s{ic, 31, "POL_CH2", 2000.0f};
    EXPECT_EQ(31u, s.id());
    EXPECT_STREQ("POL_CH2", s.name());
}

TEST(ProximeterPololu5472Test, Alarm_Instant) {
    FakeInputCaptureHAL ic;
    ic.pulse = 1500;  // > 1000 seuil
    ProximeterPololu5472 over{ic, 30, "POL", 1000.0f};
    over.read();
    EXPECT_TRUE(over.isAlarm());

    ProximeterPololu5472 under{ic, 30, "POL", 2000.0f};
    under.read();
    EXPECT_FALSE(under.isAlarm());
}

// Délégation vers l'IInputCaptureHAL injecté : bind/trigger sont no-op, doneFlag == 0
TEST(ProximeterPololu5472Test, DelegatesToInputCaptureHal) {
    FakeInputCaptureHAL ic;
    ProximeterPololu5472 s{ic, 30, "POL", 1000.0f};

    s.bind();
    s.trigger();
    EXPECT_EQ(0u, s.doneFlag());
    EXPECT_FALSE(s.isActive());
}

// ── ProximeterPololu5472 — alarme temporelle (periodWindowMs) ────────────────

TEST(ProximeterPololu5472Test, Period_NoAlarm_BeforeDurationElapsed) {
    FakeInputCaptureHAL ic;
    ic.pulse = 1500;  // > 1000 seuil
    ProximeterPololu5472 s{ic, 30, "POL", 1000.0f, 1000};
    setMockTick(0);   s.read();
    setMockTick(999);
    EXPECT_FALSE(s.isAlarm());
}

TEST(ProximeterPololu5472Test, Period_Alarm_AfterDurationElapsed) {
    FakeInputCaptureHAL ic;
    ic.pulse = 1500;
    ProximeterPololu5472 s{ic, 30, "POL", 1000.0f, 1000};
    setMockTick(0);    s.read();  // rising edge à t=0
    setMockTick(500);  s.read();
    setMockTick(1000);
    EXPECT_TRUE(s.isAlarm());
}

TEST(ProximeterPololu5472Test, Period_NoAlarm_AfterValueDrops) {
    FakeInputCaptureHAL ic;
    ic.pulse = 1500;
    ProximeterPololu5472 s{ic, 30, "POL", 1000.0f, 1000};
    setMockTick(0);    s.read();   // rising edge
    setMockTick(500);
    ic.pulse = 0;      s.read();   // repasse sous le seuil → reset
    setMockTick(1500);
    EXPECT_FALSE(s.isAlarm());
}
