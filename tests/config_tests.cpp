#include <gtest/gtest.h>

#include "data/config_manager.h"

using namespace breakout;

TEST(ConfigManagerTest, ValidConfigLoads) {
    ConfigManager mgr;
    GameConfig cfg;
    QString error;
    // Write a temp file
    QDir().mkpath("config");
    QFile f("config/test_valid.config");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << 5 << "\n" << 42 << "\n" << 2 << "\n";
    f.close();

    EXPECT_TRUE(mgr.loadConfig("test_valid", cfg, &error)) << error.toStdString();
    EXPECT_EQ(cfg.ballSpeed, 5);
    EXPECT_EQ(cfg.randomSeed, 42);
    EXPECT_EQ(cfg.startingLevel, 2);
}

TEST(ConfigManagerTest, InvalidSpeedRejected) {
    ConfigManager mgr;
    GameConfig cfg;
    QString error;
    QDir().mkpath("config");
    QFile f("config/test_invalid.config");
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << 20 << "\n" << 1 << "\n" << 1 << "\n";
    f.close();

    EXPECT_FALSE(mgr.loadConfig("test_invalid", cfg, &error));
}

TEST(ConfigManagerTest, MissingFileFails) {
    ConfigManager mgr;
    GameConfig cfg;
    QString error;
    EXPECT_FALSE(mgr.loadConfig("no_such_file", cfg, &error));
}
