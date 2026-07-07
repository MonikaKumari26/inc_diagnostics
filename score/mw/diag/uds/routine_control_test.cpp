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

/// @file routine_control_test.cpp
/// @brief Unit tests for score/mw/diag/uds/routine_control.h
///        Covers: Default RoutineControl::CompletionPercentage() implementation.

#include "score/mw/diag/uds/routine_control.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{

TEST(RoutineControlTest, CompletionPercentageDefaultReturnsNullopt)
{
    // Concrete subclass that does not override CompletionPercentage —
    // relies on the default implementation.
    struct ConcreteRoutineControl final : public RoutineControl
    {
        Result<StartRoutine> Start(std::optional<ByteView> /*input*/) override
        {
            return StartRoutine{};
        }
        Result<std::optional<ByteVector>> Stop(std::optional<ByteView> /*input*/) override
        {
            return std::optional<ByteVector>{};
        }
    };

    ConcreteRoutineControl rc{};
    EXPECT_FALSE(rc.CompletionPercentage().has_value());
}

}  // namespace score::mw::diag::uds
