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

#include "score/mw/diag/uds/serialization.h"

#include <gmock/gmock.h>

#include <cstdint>
#include <optional>

namespace score::mw::diag::uds
{

/// Mock for score::mw::diag::uds::Serializable.
/// Use when you need to inject a serializable object and control or verify
/// what serialize() returns without a real data type.
///
/// @note Does not provide the required `from_bytes()` static factory, so it
///       cannot be used as a `DataPayload` template argument for
///       `SerializedWriteDataByIdentifier` or `SerializedRoutineControl`.
///       Use it directly wherever a `const Serializable&` or a
///       `SerializedReadDataByIdentifier<SerializableMock>` is needed.
class SerializableMock : public Serializable
{
  public:
    MOCK_METHOD(Result<ByteVector>, serialize, (), (const, override));
};

/// Mock for score::mw::diag::uds::WriteHandler<DataPayload>.
/// Use when testing components that accept a WriteHandler by pointer/reference
/// and you want to verify handle_write() is called with the correct argument.
template <typename DataPayload>
class WriteHandlerMock : public WriteHandler<DataPayload>
{
  public:
    MOCK_METHOD(ResultBlank, handle_write, (DataPayload value), (override));
};

/// Mock for score::mw::diag::uds::RoutineHandler<DataPayload>.
/// All four virtual methods are mocked.
///
/// When a method is called without a matching EXPECT_CALL or ON_CALL, GMock
/// logs an "uninteresting call" warning and returns a default-constructed
/// value — it does NOT call the base-class implementation.
/// To delegate to the real default behaviour (SubFunctionNotSupported /
/// nullopt), set an explicit default action:
/// @code
///   ON_CALL(*mock, start(_)).WillByDefault(testing::CallRealMethod());
/// @endcode
template <typename DataPayload>
class RoutineHandlerMock : public RoutineHandler<DataPayload>
{
  public:
    // Parentheses around the return type are required by MOCK_METHOD when the
    // type contains a comma (template argument separator).
    MOCK_METHOD((Result<std::optional<DataPayload>>), start, (std::optional<DataPayload> params), (override));
    MOCK_METHOD((Result<std::optional<DataPayload>>), stop, (std::optional<DataPayload> params), (override));
    MOCK_METHOD((Result<std::optional<DataPayload>>), results, (), (const, override));
    MOCK_METHOD(std::optional<std::uint8_t>, completion_percentage, (), (const, noexcept, override));
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_SERIALIZATION_MOCK_H
