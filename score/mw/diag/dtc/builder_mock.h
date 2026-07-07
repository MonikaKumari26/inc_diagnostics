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

/// @file builder_mock.h
/// @brief GMock implementation of score::mw::diag::dtc::Builder.

#ifndef SCORE_MW_DIAG_DTC_BUILDER_MOCK_H
#define SCORE_MW_DIAG_DTC_BUILDER_MOCK_H

#include "score/mw/diag/dtc/builder.h"

#include <gmock/gmock.h>

namespace score::mw::diag::dtc::test
{

/// Mock for score::mw::diag::dtc::Builder.
class BuilderMock : public Builder
{
  public:
    /// @brief Default-constructs and installs a default action for Build() that returns
    ///        a default-constructed DTCMock, so tests that do not have to care about
    ///        Build() and do not need an explicit ON_CALL / EXPECT_CALL themselves.
    BuilderMock();

    // NOLINTBEGIN(readability-identifier-naming) -- MOCK_METHOD is a gmock macro, not a method name
    MOCK_METHOD(BuilderMock&, WithMonitor, (MonitorIdentifier monitor), (override));
    MOCK_METHOD(BuilderMock&, WithEvent, (EventIdentifier event), (override));
    MOCK_METHOD(BuilderMock&, WithClearCondition, (ConditionIdentifier condition), (override));
    MOCK_METHOD(BuilderMock&, ConfigureClearBehaviour, (ClearBehaviour behaviour), (override));
    MOCK_METHOD(BuilderMock&, ConfigureDebouncing, (Debounce debouncing), (override));
    MOCK_METHOD((std::unique_ptr<DTC>), Build, (), (override));
    // NOLINTEND(readability-identifier-naming)
};

}  // namespace score::mw::diag::dtc::test

#endif  // SCORE_MW_DIAG_DTC_BUILDER_MOCK_H
