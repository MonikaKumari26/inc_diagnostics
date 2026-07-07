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

/// @file serialization_test.cpp
/// @brief Unit tests for score/mw/diag/uds/serialization.h
///
/// Covers:
///   - RoutineHandler<T>                    — default start/stop/results/completion_percentage
///   - SerializedReadDataByIdentifier<T>    — Read() delegates to serialize()
///   - SerializedWriteDataByIdentifier<T,H> — Write() parse path and handler delegation
///   - SerializedRoutineControl<T,H>        — Start()/Stop()/CompletionPercentage() paths
///   - deserialize_request<T>()             — parse success, parse failure, handler error

#include "score/mw/diag/uds/serialization.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{
namespace
{

// ── Test helpers ─────────────────────────────────────────────────────────────

/// Minimal Serializable that serializes to a single byte equal to `value`.
/// from_bytes() fails on empty input.
struct TestPayload : public Serializable
{
    explicit TestPayload(std::uint8_t val = 0U) noexcept : value{val} {}
    std::uint8_t value{0U};

    Result<ByteVector> serialize() const override
    {
        return ByteVector{std::byte{value}};
    }

    static Result<TestPayload> from_bytes(ByteView data)
    {
        if (data.empty())
        {
            return {score::unexpect, NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat};
        }
        return TestPayload{static_cast<std::uint8_t>(data[0])};
    }
};

/// Serializable whose serialize() always returns ConditionsNotCorrect.
struct AlwaysFailSerialize : public Serializable
{
    Result<ByteVector> serialize() const override
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }

    static Result<AlwaysFailSerialize> from_bytes(ByteView /*data*/) { return AlwaysFailSerialize{}; }
};

/// Serializable whose from_bytes() always returns ConditionsNotCorrect.
struct AlwaysFailParse : public Serializable
{
    Result<ByteVector> serialize() const override { return ByteVector{}; }

    static Result<AlwaysFailParse> from_bytes(ByteView /*data*/)
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }
};

// ── WriteHandler helpers ──────────────────────────────────────────────────────

/// WriteHandler that always returns ConditionsNotCorrect.
struct FailingWriteHandler : public WriteHandler<TestPayload>
{
    ResultBlank handle_write(TestPayload /*val*/) override
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }
};

// ── RoutineHandler helpers ────────────────────────────────────────────────────

/// RoutineHandler that succeeds for all methods.
struct SuccessRoutineHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{TestPayload{0x42U}};
    }

    Result<std::optional<TestPayload>> stop(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{TestPayload{0x43U}};
    }

    Result<std::optional<TestPayload>> results() const override
    {
        return std::optional<TestPayload>{TestPayload{0x44U}};
    }

    std::optional<std::uint8_t> completion_percentage() const noexcept override { return std::uint8_t{75U}; }
};

/// RoutineHandler whose start() returns no reply payload.
struct NoReplyStartHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
};

/// RoutineHandler whose stop() returns no reply payload.
struct NoReplyStopHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> stop(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
};

/// RoutineHandler whose start() always returns ConditionsNotCorrect.
struct FailingStartHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> start(std::optional<TestPayload> /*params*/) override
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }
};

/// RoutineHandler whose stop() always returns ConditionsNotCorrect.
struct FailingStopHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> stop(std::optional<TestPayload> /*params*/) override
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }
};

/// RoutineHandler<AlwaysFailSerialize> whose start() returns a value (serialize will fail).
struct SerializeFailOnStartHandler : public RoutineHandler<AlwaysFailSerialize>
{
    Result<std::optional<AlwaysFailSerialize>> start(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{AlwaysFailSerialize{}};
    }
};

/// RoutineHandler<AlwaysFailSerialize> whose stop() returns a value (serialize will fail).
struct SerializeFailOnStopHandler : public RoutineHandler<AlwaysFailSerialize>
{
    Result<std::optional<AlwaysFailSerialize>> stop(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{AlwaysFailSerialize{}};
    }
};

/// Default-behaviour RoutineHandler for AlwaysFailParse — no method overrides needed
/// because parse failure short-circuits before the handler is ever called.
struct DefaultAlwaysFailParseHandler final : RoutineHandler<AlwaysFailParse>
{
};

/// Default-behaviour RoutineHandler for TestPayload — no overrides; all methods
/// return SubFunctionNotSupported. Used to test the RoutineHandler default implementations.
struct DefaultRoutineHandler final : RoutineHandler<TestPayload>
{
};

/// RoutineHandler that starts successfully (no reply) but whose results() returns an error.
/// Tests the result_provider error branch.
struct ErrorOnResultsHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
    Result<std::optional<TestPayload>> results() const override
    {
        return {score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
    }
};

/// RoutineHandler that starts successfully and whose results() returns Ok(nullopt),
/// modelling a routine that is still running.
struct NulloptResultsHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
    Result<std::optional<TestPayload>> results() const override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
};

/// RoutineHandler<AlwaysFailSerialize> that starts (no reply) and whose results() returns
/// a value whose serialize() will fail — tests the result_provider serialize-failure branch.
struct SerializeFailOnResultsHandler : public RoutineHandler<AlwaysFailSerialize>
{
    Result<std::optional<AlwaysFailSerialize>> start(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{std::nullopt};
    }
    Result<std::optional<AlwaysFailSerialize>> results() const override
    {
        return std::optional<AlwaysFailSerialize>{AlwaysFailSerialize{}};
    }
};

}  // namespace

// ── RoutineHandler<T> — default implementations ───────────────────────────────

TEST(RoutineHandlerTest, StartDefaultReturnsSubFunctionNotSupported)
{
    DefaultRoutineHandler h{};
    const auto result = h.start(std::nullopt);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SubFunctionNotSupported);
}

TEST(RoutineHandlerTest, StopDefaultReturnsSubFunctionNotSupported)
{
    DefaultRoutineHandler h{};
    const auto result = h.stop(std::nullopt);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SubFunctionNotSupported);
}

TEST(RoutineHandlerTest, ResultsDefaultReturnsSubFunctionNotSupported)
{
    const DefaultRoutineHandler h{};
    const auto result = h.results();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SubFunctionNotSupported);
}

TEST(RoutineHandlerTest, CompletionPercentageDefaultReturnsNullopt)
{
    const DefaultRoutineHandler h{};
    EXPECT_FALSE(h.completion_percentage().has_value());
}

// ── SerializedReadDataByIdentifier<T> ─────────────────────────────────────────

TEST(SerializedReadDataByIdentifierTest, ReadReturnsSerializedBytes)
{
    SerializedReadDataByIdentifier<TestPayload> reader{TestPayload{0xABU}};

    const auto result = reader.Read();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0], std::byte{0xABU});
}

TEST(SerializedReadDataByIdentifierTest, ReadPropagatesSerializeError)
{
    SerializedReadDataByIdentifier<AlwaysFailSerialize> reader{AlwaysFailSerialize{}};

    const auto result = reader.Read();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

// ── SerializedWriteDataByIdentifier<T, H> ─────────────────────────────────────

TEST(SerializedWriteDataByIdentifierTest, WriteSuccessfullyParsesAndCallsHandler)
{
    bool handler_called{false};
    std::uint8_t handler_received_value{0U};

    struct CapturingHandler final : public WriteHandler<TestPayload>
    {
        bool& called;
        std::uint8_t& received_value;
        CapturingHandler(bool& c, std::uint8_t& r) : called{c}, received_value{r} {}
        ResultBlank handle_write(TestPayload val) override
        {
            called = true;
            received_value = val.value;
            return {};
        }
    };

    CapturingHandler handler{handler_called, handler_received_value};
    SerializedWriteDataByIdentifier<TestPayload, CapturingHandler> writer{std::move(handler)};

    const ByteVector input{std::byte{0x55U}};
    const auto result = writer.Write(ByteView{input});

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(handler_received_value, 0x55U);
}

TEST(SerializedWriteDataByIdentifierTest, WriteEmptyInputReturnsIncorrectMessageLength)
{
    SerializedWriteDataByIdentifier<TestPayload, FailingWriteHandler> writer{FailingWriteHandler{}};

    const ByteVector empty{};
    const auto result = writer.Write(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedWriteDataByIdentifierTest, WriteParseFailureDoesNotCallHandler)
{
    struct NeverCalledHandler : public WriteHandler<AlwaysFailParse>
    {
        ResultBlank handle_write(AlwaysFailParse /*val*/) override
        {
            ADD_FAILURE() << "handle_write must not be called on parse failure";
            return {};
        }
    };

    SerializedWriteDataByIdentifier<AlwaysFailParse, NeverCalledHandler> writer{NeverCalledHandler{}};
    const ByteVector input{std::byte{0x00U}};
    const auto result = writer.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedWriteDataByIdentifierTest, WriteHandlerErrorIsPropagated)
{
    SerializedWriteDataByIdentifier<TestPayload, FailingWriteHandler> writer{FailingWriteHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = writer.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

// ── SerializedRoutineControl<T, H> ────────────────────────────────────────────

TEST(SerializedRoutineControlTest, StartWithNoInputAndNoReplyPayload)
{
    SerializedRoutineControl<TestPayload, NoReplyStartHandler> ctrl{NoReplyStartHandler{}};

    const auto result = ctrl.Start(std::nullopt);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->reply.has_value());
    EXPECT_NE(result->result_provider, nullptr);
}

TEST(SerializedRoutineControlTest, StartWithInputParsesAndReturnsSerializedReply)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = ctrl.Start(ByteView{input});

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->reply.has_value());
    EXPECT_EQ((*result->reply)[0], std::byte{0x42U});
}

TEST(SerializedRoutineControlTest, StartInputParseFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailParse, DefaultAlwaysFailParseHandler> ctrl{
        DefaultAlwaysFailParseHandler{}};

    const ByteVector input{std::byte{0x00U}};
    const auto result = ctrl.Start(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StartHandlerErrorIsPropagated)
{
    SerializedRoutineControl<TestPayload, FailingStartHandler> ctrl{FailingStartHandler{}};

    const auto result = ctrl.Start(std::nullopt);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StartSerializeReplyFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailSerialize, SerializeFailOnStartHandler> ctrl{
        SerializeFailOnStartHandler{}};

    const auto result = ctrl.Start(std::nullopt);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, ResultProviderCallsHandlerResultsAndSerializes)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const auto start_result = ctrl.Start(std::nullopt);
    ASSERT_TRUE(start_result.has_value());
    ASSERT_NE(start_result->result_provider, nullptr);

    const auto poll_result = start_result->result_provider();

    ASSERT_TRUE(poll_result.has_value());
    ASSERT_TRUE(poll_result->has_value());
    EXPECT_EQ((**poll_result)[0], std::byte{0x44U});
}

TEST(SerializedRoutineControlTest, ResultProviderReturnsErrorWhenResultsFails)
{
    SerializedRoutineControl<TestPayload, ErrorOnResultsHandler> ctrl{ErrorOnResultsHandler{}};

    const auto start_result = ctrl.Start(std::nullopt);
    ASSERT_TRUE(start_result.has_value());

    const auto poll_result = start_result->result_provider();

    EXPECT_FALSE(poll_result.has_value());
    EXPECT_EQ(poll_result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, ResultProviderReturnsNulloptWhenRoutineStillRunning)
{
    SerializedRoutineControl<TestPayload, NulloptResultsHandler> ctrl{NulloptResultsHandler{}};

    const auto start_result = ctrl.Start(std::nullopt);
    ASSERT_TRUE(start_result.has_value());

    const auto poll_result = start_result->result_provider();

    ASSERT_TRUE(poll_result.has_value());
    EXPECT_FALSE(poll_result->has_value());
}

TEST(SerializedRoutineControlTest, ResultProviderReturnsErrorWhenSerializeFails)
{
    SerializedRoutineControl<AlwaysFailSerialize, SerializeFailOnResultsHandler> ctrl{
        SerializeFailOnResultsHandler{}};

    const auto start_result = ctrl.Start(std::nullopt);
    ASSERT_TRUE(start_result.has_value());

    const auto poll_result = start_result->result_provider();

    EXPECT_FALSE(poll_result.has_value());
    EXPECT_EQ(poll_result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StopWithNoInputReturnsSerializedReply)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const auto result = ctrl.Stop(std::nullopt);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ((**result)[0], std::byte{0x43U});
}

TEST(SerializedRoutineControlTest, StopWithNoReplyPayloadReturnsNullopt)
{
    SerializedRoutineControl<TestPayload, NoReplyStopHandler> ctrl{NoReplyStopHandler{}};

    const auto result = ctrl.Stop(std::nullopt);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value());
}

TEST(SerializedRoutineControlTest, StopInputParseFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailParse, DefaultAlwaysFailParseHandler> ctrl{
        DefaultAlwaysFailParseHandler{}};

    const ByteVector input{std::byte{0x00U}};
    const auto result = ctrl.Stop(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StopHandlerErrorIsPropagated)
{
    SerializedRoutineControl<TestPayload, FailingStopHandler> ctrl{FailingStopHandler{}};

    const auto result = ctrl.Stop(std::nullopt);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StopSerializeReplyFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailSerialize, SerializeFailOnStopHandler> ctrl{
        SerializeFailOnStopHandler{}};

    const auto result = ctrl.Stop(std::nullopt);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, CompletionPercentageDelegatesToHandler)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const auto pct = ctrl.CompletionPercentage();

    ASSERT_TRUE(pct.has_value());
    EXPECT_EQ(*pct, 75U);
}

// ── deserialize_request<T>() ──────────────────────────────────────────────────

TEST(DeserializeRequestTest, SuccessfulParseInvokesCallable)
{
    TestPayload received{};
    bool called{false};

    const ByteVector input{std::byte{0x99U}};
    const auto result =
        deserialize_request<TestPayload>(ByteView{input}, [&](TestPayload val) -> ResultBlank {
            called = true;
            received = val;
            return {};
        });

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(called);
    EXPECT_EQ(received.value, 0x99U);
}

TEST(DeserializeRequestTest, ParseFailureReturnsIncorrectMessageLengthAndSkipsCallable)
{
    bool called{false};

    const ByteVector empty{};
    const auto result =
        deserialize_request<TestPayload>(ByteView{empty}, [&](TestPayload /*val*/) -> ResultBlank {
            called = true;
            return {};
        });

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
    EXPECT_FALSE(called);
}

TEST(DeserializeRequestTest, CallableErrorIsPropagated)
{
    const ByteVector input{std::byte{0x01U}};

    const auto result =
        deserialize_request<TestPayload>(ByteView{input}, [](TestPayload /*val*/) -> ResultBlank {
            return {score::unexpect, NegativeResponseCode::SecurityAccessDenied};
        });

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SecurityAccessDenied);
}

}  // namespace score::mw::diag::uds
