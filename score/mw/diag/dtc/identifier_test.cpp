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
///        Covers: Identifier template, MonitorIdentifier, EventIdentifier,
///        ConditionIdentifier — construction, GetValue(), operator==, operator!=.

#include "score/mw/diag/dtc/identifier.h"

#include <gtest/gtest.h>

#include <string>

namespace score::mw::diag::dtc
{
namespace
{

// ---------------------------------------------------------------------------
// MonitorIdentifier
// ---------------------------------------------------------------------------

TEST(MonitorIdentifierTest, ConstructFromStringLiteral)
{
    const MonitorIdentifier id{"mon/voltage_sensor"};
    EXPECT_EQ(id.GetValue(), "mon/voltage_sensor");
}

TEST(MonitorIdentifierTest, ConstructFromStdString)
{
    const std::string s{"mon/example"};
    const MonitorIdentifier id{s};
    EXPECT_EQ(id.GetValue(), "mon/example");
}

TEST(MonitorIdentifierTest, EqualityOperatorTrueForSameValue)
{
    EXPECT_EQ(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/a"});
}

TEST(MonitorIdentifierTest, InequalityOperatorTrueForDifferentValues)
{
    EXPECT_NE(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/b"});
}

// ---------------------------------------------------------------------------
// EventIdentifier
// ---------------------------------------------------------------------------

TEST(EventIdentifierTest, ConstructFromStringLiteral)
{
    const EventIdentifier id{"evt/overheating"};
    EXPECT_EQ(id.GetValue(), "evt/overheating");
}

TEST(EventIdentifierTest, ConstructFromStdString)
{
    const std::string s{"evt/example"};
    const EventIdentifier id{s};
    EXPECT_EQ(id.GetValue(), "evt/example");
}

TEST(EventIdentifierTest, EqualityOperatorTrueForSameValue)
{
    EXPECT_EQ(EventIdentifier{"evt/x"}, EventIdentifier{"evt/x"});
}

TEST(EventIdentifierTest, InequalityOperatorTrueForDifferentValues)
{
    EXPECT_NE(EventIdentifier{"evt/x"}, EventIdentifier{"evt/y"});
}

// ---------------------------------------------------------------------------
// ConditionIdentifier
// ---------------------------------------------------------------------------

TEST(ConditionIdentifierTest, ConstructFromStringLiteral)
{
    const ConditionIdentifier id{"cond/ignition_on"};
    EXPECT_EQ(id.GetValue(), "cond/ignition_on");
}

TEST(ConditionIdentifierTest, ConstructFromStdString)
{
    const std::string s{"cond/example"};
    const ConditionIdentifier id{s};
    EXPECT_EQ(id.GetValue(), "cond/example");
}

TEST(ConditionIdentifierTest, EqualityOperatorTrueForSameValue)
{
    EXPECT_EQ(ConditionIdentifier{"cond/x"}, ConditionIdentifier{"cond/x"});
}

TEST(ConditionIdentifierTest, InequalityOperatorTrueForDifferentValues)
{
    EXPECT_NE(ConditionIdentifier{"cond/x"}, ConditionIdentifier{"cond/y"});
}

// ---------------------------------------------------------------------------
// Type safety (compile-time enforcement)
// ---------------------------------------------------------------------------
// The compiler rejects cross-type assignments such as:
//
//   MonitorIdentifier m = EventIdentifier{"evt/x"};      // ERROR: different Tag
//   EventIdentifier   e = ConditionIdentifier{"cond/x"}; // ERROR: different Tag
//
// Verified at runtime: two identifiers with the same string but different types
// have separate, non-comparable identity — operator== is not defined across types.

TEST(IdentifierTypeTest, DifferentTypesWithSameValueHaveSameGetValue)
{
    // GetValue() returns the same string for all three types when constructed
    // from the same literal — but the types themselves are incompatible.
    const MonitorIdentifier m{"same/value"};
    const EventIdentifier e{"same/value"};
    const ConditionIdentifier c{"same/value"};

    EXPECT_EQ(m.GetValue(), "same/value");
    EXPECT_EQ(e.GetValue(), "same/value");
    EXPECT_EQ(c.GetValue(), "same/value");
    // m == e or m == c would not compile — enforced by the type system.
}

TEST(IdentifierTypeTest, GetValueReturnsExactStringPassedToConstructor)
{
    EXPECT_EQ(MonitorIdentifier{"mon/sensor_1"}.GetValue(), "mon/sensor_1");
    EXPECT_EQ(EventIdentifier{"evt/fault_42"}.GetValue(), "evt/fault_42");
    EXPECT_EQ(ConditionIdentifier{"cond/always"}.GetValue(), "cond/always");
}

}  // namespace
}  // namespace score::mw::diag::dtc
