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

/// @file debounce_test.cpp
/// @brief Unit tests for score/mw/diag/dtc/debounce.h
///        Covers: Debounce::Timer IsValid; Debounce::Counter IsValid;
///                Debounce IsSet, GetAlgorithm.

#include "score/mw/diag/dtc/debounce.h"

#include <gtest/gtest.h>

namespace score::mw::diag::dtc
{
namespace
{

// ---------------------------------------------------------------------------
// Timer — IsValid
// ---------------------------------------------------------------------------

TEST(TimerTest, IsValidReturnsTrueForPositiveDurations)
{
    EXPECT_TRUE((Debounce::Timer{200U, 100U}.IsValid()));
}

TEST(TimerTest, IsValidReturnsFalseWhenFailedMsIsZero)
{
    EXPECT_FALSE((Debounce::Timer{0U, 100U}.IsValid()));
}

TEST(TimerTest, IsValidReturnsFalseWhenPassedMsIsZero)
{
    EXPECT_FALSE((Debounce::Timer{200U, 0U}.IsValid()));
}

// ---------------------------------------------------------------------------
// Counter — IsValid
// ---------------------------------------------------------------------------

TEST(CounterTest, IsValidReturnsTrueForValidConfig)
{
    EXPECT_TRUE((Debounce::Counter{10, -5, 2U, 1U, 0, 0, false, false}.IsValid()));
}

TEST(CounterTest, IsValidReturnsFalseWhenFailedThresholdIsZero)
{
    EXPECT_FALSE((Debounce::Counter{0, -5, 2U, 1U, 0, 0, false, false}.IsValid()));
}

TEST(CounterTest, IsValidReturnsFalseWhenPassedThresholdIsZero)
{
    EXPECT_FALSE((Debounce::Counter{10, 0, 2U, 1U, 0, 0, false, false}.IsValid()));
}

TEST(CounterTest, IsValidReturnsFalseWhenFailedStepsizeIsZero)
{
    EXPECT_FALSE((Debounce::Counter{10, -5, 0U, 1U, 0, 0, false, false}.IsValid()));
}

TEST(CounterTest, IsValidReturnsFalseWhenPassedStepsizeIsZero)
{
    EXPECT_FALSE((Debounce::Counter{10, -5, 2U, 0U, 0, 0, false, false}.IsValid()));
}

// ---------------------------------------------------------------------------
// Debounce — IsSet, GetAlgorithm, operator==
// ---------------------------------------------------------------------------

TEST(DebounceTest, DefaultConstructedIsNotSet) { EXPECT_FALSE(Debounce{}.IsSet()); }

TEST(DebounceTest, IsSetTrueWhenConstructedWithTimer)
{
    EXPECT_TRUE((Debounce{Debounce::Timer{200U, 100U}}.IsSet()));
}

TEST(DebounceTest, IsSetTrueWhenConstructedWithCounter)
{
    EXPECT_TRUE((Debounce{Debounce::Counter{10, -5, 2U, 1U, 0, 0, false, false}}.IsSet()));
}

TEST(DebounceTest, GetAlgorithmHoldsTimerWhenConstructedWithTimer)
{
    const Debounce d{Debounce::Timer{200U, 100U}};
    ASSERT_TRUE(std::holds_alternative<Debounce::Timer>(d.GetAlgorithm()));
    const auto& t = std::get<Debounce::Timer>(d.GetAlgorithm());
    EXPECT_EQ(t.failed_ms, 200U);
    EXPECT_EQ(t.passed_ms, 100U);
}

TEST(DebounceTest, GetAlgorithmHoldsCounterWhenConstructedWithCounter)
{
    const Debounce::Counter cfg{10, -5, 2U, 1U, 0, 0, false, false};
    const Debounce d{cfg};
    ASSERT_TRUE(std::holds_alternative<Debounce::Counter>(d.GetAlgorithm()));
    const auto& c = std::get<Debounce::Counter>(d.GetAlgorithm());
    EXPECT_EQ(c.failed_threshold, 10);
    EXPECT_EQ(c.passed_threshold, -5);
    EXPECT_EQ(c.failed_stepsize, 2U);
    EXPECT_EQ(c.passed_stepsize, 1U);
    EXPECT_EQ(c.failed_jump_value, 0);
    EXPECT_EQ(c.passed_jump_value, 0);
    EXPECT_FALSE(c.use_jump_to_failed);
    EXPECT_FALSE(c.use_jump_to_passed);
}

TEST(DebounceTest, GetAlgorithmHoldsMonostateWhenDefault)
{
    EXPECT_TRUE(std::holds_alternative<std::monostate>(Debounce{}.GetAlgorithm()));
}

}  // namespace
}  // namespace score::mw::diag::dtc
