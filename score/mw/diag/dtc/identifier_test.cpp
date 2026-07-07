/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

/// @file identifier_test.cpp
/// @brief Unit tests for score/mw/diag/dtc/identifier.h
///        Covers: construction from std::string_view, std::string and string literal,
///                GetValue(), operator==, operator!=, operator<, empty-string precondition.

#include "score/mw/diag/dtc/identifier.h"

#include <gtest/gtest.h>

namespace score::mw::diag::dtc
{
namespace
{

// ---------------------------------------------------------------------------
// MonitorIdentifier
// ---------------------------------------------------------------------------

TEST(MonitorIdentifierTest, ConstructFromStringView)
{
    const std::string_view sv{"mon/voltage_sensor"};
    const MonitorIdentifier id{sv};
    EXPECT_EQ(id.GetValue(), "mon/voltage_sensor");
}

TEST(MonitorIdentifierTest, ConstructFromStdString)
{
    const std::string s{"mon/example"};
    const MonitorIdentifier id{s};
    EXPECT_EQ(id.GetValue(), "mon/example");
}

TEST(MonitorIdentifierDeathTest, ConstructFromEmptyStringDies)
{
    EXPECT_DEATH(MonitorIdentifier{std::string_view{""}}, "");
}

TEST(MonitorIdentifierTest, EqualityOperatorTrueForSameValue)
{
    EXPECT_EQ(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/a"});
}

TEST(MonitorIdentifierTest, EqualityOperatorFalseForDifferentValues)
{
    EXPECT_FALSE(MonitorIdentifier{"mon/a"} == MonitorIdentifier{"mon/b"});
}

TEST(MonitorIdentifierTest, InequalityOperatorTrueForDifferentValues)
{
    EXPECT_NE(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/b"});
}

TEST(MonitorIdentifierTest, InequalityOperatorFalseForSameValue)
{
    EXPECT_FALSE(MonitorIdentifier{"mon/a"} != MonitorIdentifier{"mon/a"});
}

TEST(MonitorIdentifierTest, LessThanOperatorReflectsLexicographicOrder)
{
    EXPECT_LT(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/b"});
    EXPECT_FALSE(MonitorIdentifier{"mon/b"} < MonitorIdentifier{"mon/a"});
    EXPECT_FALSE(MonitorIdentifier{"mon/a"} < MonitorIdentifier{"mon/a"});
}

}  // namespace
}  // namespace score::mw::diag::dtc
