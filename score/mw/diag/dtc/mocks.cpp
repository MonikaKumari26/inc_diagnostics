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

#include "score/mw/diag/dtc/builder_mock.h"
#include "score/mw/diag/dtc/dtc_mock.h"

namespace score::mw::diag::dtc::test
{

// NOLINTNEXTLINE(modernize-use-equals-default) -- body installs ON_CALL default action
BuilderMock::BuilderMock()
{
    ON_CALL(*this, Build())
        .WillByDefault([]() -> std::unique_ptr<DTC> { return std::make_unique<DTCMock>(); });
}

}  // namespace score::mw::diag::dtc::test
