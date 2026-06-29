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

namespace score::mw::diag::dtc
{

/// Mock for score::mw::diag::dtc::Builder.
class BuilderMock : public Builder
{
  public:
    MOCK_METHOD(BuilderMock&, WithMonitor, (MonitorIdentifier), (override));
    MOCK_METHOD(BuilderMock&, WithEvent, (EventIdentifier), (override));
    MOCK_METHOD(BuilderMock&, WithClearCondition, (ConditionIdentifier), (override));
    MOCK_METHOD(BuilderMock&, ConfigureAsNotClearable, (), (override));
    MOCK_METHOD(BuilderMock&, ConfigureAsReenterAfterCleared, (), (override));
    MOCK_METHOD(BuilderMock&, ConfigureDebouncing, (Debounce), (override));
    MOCK_METHOD((std::unique_ptr<DTC>), Build, (), (override));
};

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_BUILDER_MOCK_H
