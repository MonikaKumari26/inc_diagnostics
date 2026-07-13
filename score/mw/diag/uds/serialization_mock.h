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

/// @file serialization_mock.h
/// @brief GMock implementations of score::mw::diag::uds::Serializable,
///        WriteHandler<T>, and RoutineHandler<T>.

#ifndef SCORE_MW_DIAG_UDS_SERIALIZATION_MOCK_H
#define SCORE_MW_DIAG_UDS_SERIALIZATION_MOCK_H

#include "score/mw/diag/uds/serialization_base.h"

#include <gmock/gmock.h>

#include <cstdint>
#include <optional>

namespace score::mw::diag::uds::test
{

/// Mock for score::mw::diag::uds::Serializable.
class SerializableMock : public Serializable
{
  public:
    MOCK_METHOD(Result<ByteVector>, Serialize, (), (const, override));
};

/// Mock for score::mw::diag::uds::WriteHandler<DataPayload>.
template<typename DataPayload> class WriteHandlerMock : public WriteHandler<DataPayload>
{
  public:
    MOCK_METHOD((Result<score::cpp::blank>), HandleWrite, (DataPayload value), (override));
};

/// Mock for score::mw::diag::uds::RoutineHandler<DataPayload>.
template<typename DataPayload> class RoutineHandlerMock : public RoutineHandler<DataPayload>
{
  public:
    MOCK_METHOD((Result<std::optional<DataPayload>>), Start, (std::optional<DataPayload> params), (override));
    MOCK_METHOD((Result<std::optional<DataPayload>>), Stop, (std::optional<DataPayload> params), (override));
    MOCK_METHOD(std::optional<std::uint8_t>, CompletionPercentage, (), (const, noexcept, override));
};

}  // namespace score::mw::diag::uds::test

#endif  // SCORE_MW_DIAG_UDS_SERIALIZATION_MOCK_H
