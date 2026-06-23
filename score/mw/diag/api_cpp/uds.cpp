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

/// @file uds.cpp
/// @brief destructor definitions for UDS interface classes.

#include "score/mw/diag/uds.h"

namespace score
{
namespace mw
{
namespace diag
{

ReadDataByIdentifier::~ReadDataByIdentifier() noexcept = default;

WriteDataByIdentifier::~WriteDataByIdentifier() noexcept = default;

GenericDataIdentifier::~GenericDataIdentifier() noexcept = default;

RoutineControl::~RoutineControl() noexcept = default;

// Default implementation rejects with SubFunctionNotSupported —
Result<ByteVector> UdsService::handle_message(ByteView /*input*/)
{
    return Result<ByteVector>{score::unexpect,
        Error::from_nrc(uds::NegativeResponseCode::SubFunctionNotSupported)};
}

UdsService::~UdsService() noexcept = default;

}  // namespace diag
}  // namespace mw
}  // namespace score