#include <gtest/gtest.h>
#include "model/ModelRegistry.hpp"

TEST(ModelRegistryTest, StartsEmpty)
{
    ModelRegistry registry;

    EXPECT_FALSE(registry.checkModel("dummy mode"));
    EXPECT_FALSE(registry.checkVersion("dummy mode", "v1"));
}

TEST(ModelRegistryTest, AddModelCreatesModelAndVersion)
{
    ModelRegistry registry;

    bool added = registry.addModel("dummy mode", "v1");

    EXPECT_TRUE(added);
    EXPECT_TRUE(registry.checkModel("dummy mode"));
    EXPECT_TRUE(registry.checkVersion("dummy mode", "v1"));
}

TEST(ModelRegistryTest, RejectsDuplicateModelVersion)
{
    ModelRegistry registry;

    bool first_add = registry.addModel("dummy mode", "v1");
    bool second_add = registry.addModel("dummy mode", "v1");

    EXPECT_TRUE(first_add);
    EXPECT_FALSE(second_add);
}

TEST(ModelRegistryTest, SameModelCanHaveMultipleVersions)
{
    ModelRegistry registry;

    EXPECT_TRUE(registry.addModel("dummy mode", "v1"));
    EXPECT_TRUE(registry.addModel("dummy mode", "v2"));

    EXPECT_TRUE(registry.checkModel("dummy mode"));
    EXPECT_TRUE(registry.checkVersion("dummy mode", "v1"));
    EXPECT_TRUE(registry.checkVersion("dummy mode", "v2"));
}

TEST(ModelRegistryTest, DifferentModelsCanHaveSameVersionName)
{
    ModelRegistry registry;

    EXPECT_TRUE(registry.addModel("dummy mode", "v1"));
    EXPECT_TRUE(registry.addModel("other model", "v1"));

    EXPECT_TRUE(registry.checkVersion("dummy mode", "v1"));
    EXPECT_TRUE(registry.checkVersion("other model", "v1"));
}

TEST(ModelRegistryTest, UnknownVersionReturnsFalse)
{
    ModelRegistry registry;

    registry.addModel("dummy mode", "v1");

    EXPECT_FALSE(registry.checkVersion("dummy mode", "v2"));
}

TEST(ModelRegistryTest, UnknownModelReturnsFalse)
{
    ModelRegistry registry;

    registry.addModel("dummy mode", "v1");

    EXPECT_FALSE(registry.checkModel("missing model"));
    EXPECT_FALSE(registry.checkVersion("missing model", "v1"));
}