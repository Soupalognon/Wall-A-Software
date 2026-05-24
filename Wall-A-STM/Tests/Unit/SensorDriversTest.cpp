#include <gtest/gtest.h>
#include "Stubs/HalStub.h"
#include "Mocks/MockAnalogSource.h"
#include "Services/AnalogSensor.h"

// ── AnalogSensor — température ───────────────────────────────────────────────

TEST(AnalogSensorTest, Read_ReturnsPrimaryChannel) {
    MockAnalogSource src;
    src.values[0] = 45.0f;
    AnalogSensor sensor{4, "TEMP_PRI", &src, 0, 60.0f};
    EXPECT_FLOAT_EQ(45.0f, sensor.read());
}

TEST(AnalogSensorTest, Read_ThreeChannelsAreIndependent) {
    MockAnalogSource src;
    src.values[0] = 10.0f;
    src.values[1] = 20.0f;
    src.values[2] = 30.0f;
    AnalogSensor s0{4, "TEMP_PRI", &src, 0, 60.0f};
    AnalogSensor s1{5, "TEMP_SEC", &src, 1, 60.0f};
    AnalogSensor s2{6, "TEMP_PWR", &src, 2, 60.0f};
    EXPECT_FLOAT_EQ(10.0f, s0.read());
    EXPECT_FLOAT_EQ(20.0f, s1.read());
    EXPECT_FLOAT_EQ(30.0f, s2.read());
}

TEST(AnalogSensorTest, Alarm_WhenOverThreshold) {
    MockAnalogSource src;
    src.values[1] = 70.0f;
    AnalogSensor sensor{5, "TEMP_SEC", &src, 1, 60.0f};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(AnalogSensorTest, NoAlarm_WhenUnderThreshold) {
    MockAnalogSource src;
    src.values[2] = 40.0f;
    AnalogSensor sensor{6, "TEMP_PWR", &src, 2, 60.0f};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}

TEST(AnalogSensorTest, IdAndName_AreCorrect) {
    MockAnalogSource src;
    AnalogSensor s0{4, "TEMP_PRI", &src, 0, 60.0f};
    AnalogSensor s1{5, "TEMP_SEC", &src, 1, 60.0f};
    AnalogSensor s2{6, "TEMP_PWR", &src, 2, 60.0f};
    EXPECT_EQ(4u, s0.id());  EXPECT_STREQ("TEMP_PRI", s0.name());
    EXPECT_EQ(5u, s1.id());  EXPECT_STREQ("TEMP_SEC", s1.name());
    EXPECT_EQ(6u, s2.id());  EXPECT_STREQ("TEMP_PWR", s2.name());
}

// ── AnalogSensor — courant moteur ────────────────────────────────────────────

TEST(AnalogSensorTest, Read_ReturnsPrimaryLeftChannel) {
    MockAnalogSource src;
    src.values[0] = 1.2f;
    AnalogSensor sensor{7, "CUR_PL", &src, 0, 2.0f};
    EXPECT_FLOAT_EQ(1.2f, sensor.read());
}

TEST(AnalogSensorTest, Read_FourChannelsAreIndependent) {
    MockAnalogSource src;
    src.values[0] = 0.1f;
    src.values[1] = 0.2f;
    src.values[2] = 0.3f;
    src.values[3] = 0.4f;
    AnalogSensor s0{7,  "CUR_PL", &src, 0, 2.0f};
    AnalogSensor s1{8,  "CUR_PR", &src, 1, 2.0f};
    AnalogSensor s2{9,  "CUR_SL", &src, 2, 2.0f};
    AnalogSensor s3{10, "CUR_SR", &src, 3, 2.0f};
    EXPECT_FLOAT_EQ(0.1f, s0.read());
    EXPECT_FLOAT_EQ(0.2f, s1.read());
    EXPECT_FLOAT_EQ(0.3f, s2.read());
    EXPECT_FLOAT_EQ(0.4f, s3.read());
}

TEST(AnalogSensorTest, Alarm_WhenOverCurrentThreshold) {
    MockAnalogSource src;
    src.values[0] = 3.0f;
    AnalogSensor sensor{7, "CUR_PL", &src, 0, 2.0f};
    sensor.read();
    EXPECT_TRUE(sensor.isAlarm());
}

TEST(AnalogSensorTest, NoAlarm_WhenUnderCurrentThreshold) {
    MockAnalogSource src;
    src.values[1] = 1.0f;
    AnalogSensor sensor{8, "CUR_PR", &src, 1, 2.0f};
    sensor.read();
    EXPECT_FALSE(sensor.isAlarm());
}

TEST(AnalogSensorTest, IdAndName_AreCorrectForCurrentSensors) {
    MockAnalogSource src;
    AnalogSensor s0{7,  "CUR_PL", &src, 0, 2.0f};
    AnalogSensor s1{8,  "CUR_PR", &src, 1, 2.0f};
    AnalogSensor s2{9,  "CUR_SL", &src, 2, 2.0f};
    AnalogSensor s3{10, "CUR_SR", &src, 3, 2.0f};
    EXPECT_EQ(7u,  s0.id());  EXPECT_STREQ("CUR_PL", s0.name());
    EXPECT_EQ(8u,  s1.id());  EXPECT_STREQ("CUR_PR", s1.name());
    EXPECT_EQ(9u,  s2.id());  EXPECT_STREQ("CUR_SL", s2.name());
    EXPECT_EQ(10u, s3.id());  EXPECT_STREQ("CUR_SR", s3.name());
}

// ── AnalogSensor — alarme PERIOD ─────────────────────────────────────────────

TEST(AnalogSensorTest, Period_NoAlarm_BeforeDurationElapsed) {
    // Valeur au-dessus du seuil mais pas encore 1000ms écoulées
    MockAnalogSource src;
    src.values[0] = 5.0f;
    AnalogSensor s{1, "CUR", &src, 0, 2.0f, 1000};
    setMockTick(0);   s.read();
    setMockTick(999);
    EXPECT_FALSE(s.isAlarm());
}

TEST(AnalogSensorTest, Period_Alarm_AfterDurationElapsed) {
    // Valeur au-dessus du seuil pendant exactement 1000ms
    MockAnalogSource src;
    src.values[0] = 5.0f;
    AnalogSensor s{1, "CUR", &src, 0, 2.0f, 1000};
    setMockTick(0);    s.read();  // rising edge à t=0
    setMockTick(500);  s.read();  // toujours au-dessus
    setMockTick(1000);
    EXPECT_TRUE(s.isAlarm());
}

TEST(AnalogSensorTest, Period_NoAlarm_AfterValueDrops) {
    // La valeur repasse en dessous → timer reset, plus d'alarme
    MockAnalogSource src;
    src.values[0] = 5.0f;
    AnalogSensor s{1, "CUR", &src, 0, 2.0f, 1000};
    setMockTick(0);    s.read();   // rising edge
    setMockTick(500);
    src.values[0] = 1.0f;
    s.read();                      // repasse en dessous → reset
    setMockTick(1500);
    EXPECT_FALSE(s.isAlarm());
}

TEST(AnalogSensorTest, Period_RisingEdge_ResetsOnDrop) {
    // Remonte après être passé en dessous → nouveau timer
    MockAnalogSource src;
    src.values[0] = 5.0f;
    AnalogSensor s{1, "CUR", &src, 0, 2.0f, 1000};
    setMockTick(0);    s.read();   // 1er rising edge
    setMockTick(100);
    src.values[0] = 1.0f;
    s.read();                      // descend
    setMockTick(200);
    src.values[0] = 5.0f;
    s.read();                      // 2ème rising edge à t=200
    setMockTick(1100);             // 900ms après le 2ème edge → pas encore 1000ms
    EXPECT_FALSE(s.isAlarm());
    setMockTick(1200);             // 1000ms après le 2ème edge
    EXPECT_TRUE(s.isAlarm());
}

TEST(AnalogSensorTest, Period_IndependentOfRefreshRate) {
    // Même résultat avec 1 lecture ou 10 lectures pendant la fenêtre
    MockAnalogSource src;
    src.values[0] = 5.0f;

    AnalogSensor slow{1, "CUR", &src, 0, 2.0f, 2000};
    setMockTick(0);    slow.read();   // 1 seule lecture à t=0
    setMockTick(2000);
    EXPECT_TRUE(slow.isAlarm());

    AnalogSensor fast{1, "CUR", &src, 0, 2.0f, 2000};
    setMockTick(0);
    for (uint32_t t = 0; t <= 2000; t += 100) {
        setMockTick(t);
        fast.read();                  // 21 lectures sur la même fenêtre
    }
    EXPECT_TRUE(fast.isAlarm());
}
