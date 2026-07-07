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

/// @file diagnostic_services_collection_builder_test.cpp
/// @brief Unit tests for DiagnosticJobCollection, DiagnosticServicesCollection, and
///        DiagnosticServicesCollectionBuilder.
///
/// Covers:
///   DiagnosticJobCollection
///     - Virtual dispatch through base pointer (via mock).
///
///   DiagnosticServicesCollection / DiagnosticServicesCollectionBuilder
///     - build() on an empty builder succeeds (returns a non-null collection).
///     - build() with each of the five handler types succeeds.
///     - build() with all handler types combined succeeds.
///     - build() fails (PreconditionNotFulfilled → ConditionsNotCorrect) when any
///       registered handler pointer is nullptr.
///     - build() clears the builder state — a subsequent call returns an empty
///       (but valid) collection without error.
///     - with_*() methods return *this (enable chaining).
///     - The returned DiagnosticServicesCollection is a DiagnosticJobCollection.

#include "score/mw/diag/uds/diagnostic_services_collection_builder.h"

#include "score/mw/diag/uds/diagnostic_job_collection_mock.h"
#include "score/mw/diag/uds/generic_data_identifier_mock.h"
#include "score/mw/diag/uds/generic_service_mock.h"
#include "score/mw/diag/uds/read_data_by_identifier_mock.h"
#include "score/mw/diag/uds/routine_control_mock.h"
#include "score/mw/diag/uds/write_data_by_identifier_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace score::mw::diag::uds
{

// ── DiagnosticJobCollection ──────────────────────────────────────────────────

TEST(DiagnosticJobCollectionTest, VirtualDestructorFiresThroughBasePointer)
{
    // Verify that releasing a base-class unique_ptr correctly dispatches
    // to the derived destructor (proves the vtable is set up correctly).
    auto mock = std::make_unique<DiagnosticJobCollectionMock>();
    EXPECT_CALL(*mock, Destruct());

    std::unique_ptr<DiagnosticJobCollection> base = std::move(mock);
    base.reset();  // ~DiagnosticJobCollectionMock() must fire here
}

// ── DiagnosticServicesCollectionBuilder — successful build paths ─────────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildEmptyCollectionSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};

    auto result = builder.build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->get(), nullptr);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithReadDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithWriteDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_write_did("F190", std::make_unique<WriteDataByIdentifierMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithDataIdSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_data_id("F190", std::make_unique<GenericDataIdentifierMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithRoutineSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_routine("0301", std::make_unique<RoutineControlMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithUdsServiceSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_uds_service("B200", std::make_unique<GenericServiceMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithAllHandlerTypesCombinedSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>())
        .with_write_did("F191", std::make_unique<WriteDataByIdentifierMock>())
        .with_data_id("F192", std::make_unique<GenericDataIdentifierMock>())
        .with_routine("0301", std::make_unique<RoutineControlMock>())
        .with_uds_service("B200", std::make_unique<GenericServiceMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, MultipleHandlersPerTypeSuceed)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>());
    builder.with_read_did("F191", std::make_unique<ReadDataByIdentifierMock>());
    builder.with_routine("0301", std::make_unique<RoutineControlMock>());
    builder.with_routine("0302", std::make_unique<RoutineControlMock>());

    auto result = builder.build();

    EXPECT_TRUE(result.has_value());
}

// ── DiagnosticServicesCollectionBuilder — null-handler failure paths ──────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullReadDidFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", nullptr);

    auto result = builder.build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullWriteDidFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_write_did("F190", nullptr);

    auto result = builder.build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullDataIdFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_data_id("F190", nullptr);

    auto result = builder.build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullRoutineFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_routine("0301", nullptr);

    auto result = builder.build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullUdsServiceFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_uds_service("B200", nullptr);

    auto result = builder.build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, NullHandlerMixedWithValidHandlerFails)
{
    // A single null entry among multiple valid handlers should still fail.
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>())
        .with_read_did("F191", nullptr);  // null entry

    auto result = builder.build();

    EXPECT_FALSE(result.has_value());
}

// ── DiagnosticServicesCollectionBuilder — post-build state ───────────────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildClearsBuilderState)
{
    // After a successful build() the builder vectors are moved-from (empty).
    // A second build() call should therefore also succeed and return an
    // empty-but-valid collection.
    DiagnosticServicesCollectionBuilder builder{};
    builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>());

    auto result1 = builder.build();
    ASSERT_TRUE(result1.has_value());

    auto result2 = builder.build();  // builder is now empty
    EXPECT_TRUE(result2.has_value());
}

// ── DiagnosticServicesCollectionBuilder — method chaining ────────────────────

TEST(DiagnosticServicesCollectionBuilderTest, WithReadDidReturnsSameBuilder)
{
    DiagnosticServicesCollectionBuilder builder{};
    auto& returned = builder.with_read_did("F190", std::make_unique<ReadDataByIdentifierMock>());
    EXPECT_EQ(&returned, &builder);
}

TEST(DiagnosticServicesCollectionBuilderTest, WithWriteDidReturnsSameBuilder)
{
    DiagnosticServicesCollectionBuilder builder{};
    auto& returned = builder.with_write_did("F190", std::make_unique<WriteDataByIdentifierMock>());
    EXPECT_EQ(&returned, &builder);
}

TEST(DiagnosticServicesCollectionBuilderTest, WithDataIdReturnsSameBuilder)
{
    DiagnosticServicesCollectionBuilder builder{};
    auto& returned = builder.with_data_id("F190", std::make_unique<GenericDataIdentifierMock>());
    EXPECT_EQ(&returned, &builder);
}

TEST(DiagnosticServicesCollectionBuilderTest, WithRoutineReturnsSameBuilder)
{
    DiagnosticServicesCollectionBuilder builder{};
    auto& returned = builder.with_routine("0301", std::make_unique<RoutineControlMock>());
    EXPECT_EQ(&returned, &builder);
}

TEST(DiagnosticServicesCollectionBuilderTest, WithUdsServiceReturnsSameBuilder)
{
    DiagnosticServicesCollectionBuilder builder{};
    auto& returned = builder.with_uds_service("B200", std::make_unique<GenericServiceMock>());
    EXPECT_EQ(&returned, &builder);
}

// ── DiagnosticServicesCollection — type relationships ───────────────────────

TEST(DiagnosticServicesCollectionBuilderTest, CollectionIsADiagnosticJobCollection)
{
    // The returned collection must be usable as a DiagnosticJobCollection,
    // enabling callers to hold it through the base-class interface.
    DiagnosticServicesCollectionBuilder builder{};
    auto result = builder.build();
    ASSERT_TRUE(result.has_value());

    std::unique_ptr<DiagnosticJobCollection> base = std::move(*result);
    EXPECT_NE(base.get(), nullptr);
}

}  // namespace score::mw::diag::uds
