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
/// @brief Unit tests for score/mw/diag/uds/serialization_base.h and
///        score/mw/diag/uds/serialization.h

#include "score/mw/diag/uds/serialization.h"
#include "score/mw/diag/uds/meta_data.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds
{
namespace
{

// ── Test helpers ─────────────────────────────────────────────────────────────

struct TestPayload : public Serializable
{
    explicit TestPayload(std::uint8_t val = 0U) noexcept : value{val} {}
    std::uint8_t value{0U};

    Result<ByteVector> Serialize() const override { return ByteVector{std::byte{value}}; }

    static Result<TestPayload> FromBytes(ByteView data)
    {
        if (data.empty())
        {
            return Result<TestPayload>(score::cpp::make_unexpected(NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat));
        }
        return TestPayload{static_cast<std::uint8_t>(data[0])};
    }
};

struct AlwaysFailSerialize : public Serializable
{
    Result<ByteVector> Serialize() const override
    {
        return Result<ByteVector>(score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect));
    }

    static Result<AlwaysFailSerialize> FromBytes(ByteView /*data*/) { return AlwaysFailSerialize{}; }
};

struct AlwaysFailParse : public Serializable
{
    Result<ByteVector> Serialize() const override { return ByteVector{}; }

    static Result<AlwaysFailParse> FromBytes(ByteView /*data*/)
    {
        return Result<AlwaysFailParse>(score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect));
    }
};

// ── WriteHandler helpers ──────────────────────────────────────────────────────

struct FailingWriteHandler : public WriteHandler<TestPayload>
{
    Result<score::cpp::blank> HandleWrite(TestPayload /*val*/) override
    {
        return Result<score::cpp::blank>(score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect));
    }
};

// ── RoutineHandler helpers ────────────────────────────────────────────────────

struct SuccessRoutineHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{TestPayload{0x42U}};
    }

    Result<std::optional<TestPayload>> Stop(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{TestPayload{0x43U}};
    }

    std::optional<std::uint8_t> CompletionPercentage() const noexcept override { return std::uint8_t{75U}; }
};

struct NoReplyStartHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
};

struct NoReplyStopHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
    Result<std::optional<TestPayload>> Stop(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
};

struct FailingStartHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return Result<std::optional<TestPayload>>(score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect));
    }
};

struct FailingStopHandler : public RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return std::optional<TestPayload>{std::nullopt};
    }
    Result<std::optional<TestPayload>> Stop(std::optional<TestPayload> /*params*/) override
    {
        return Result<std::optional<TestPayload>>(score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect));
    }
};

struct SerializeFailOnStartHandler : public RoutineHandler<AlwaysFailSerialize>
{
    Result<std::optional<AlwaysFailSerialize>> Start(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{AlwaysFailSerialize{}};
    }
};

struct SerializeFailOnStopHandler : public RoutineHandler<AlwaysFailSerialize>
{
    Result<std::optional<AlwaysFailSerialize>> Start(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{std::nullopt};
    }
    Result<std::optional<AlwaysFailSerialize>> Stop(std::optional<AlwaysFailSerialize> /*params*/) override
    {
        return std::optional<AlwaysFailSerialize>{AlwaysFailSerialize{}};
    }
};

/// RoutineHandler for AlwaysFailParse — Start() is never reached because parse failure
/// short-circuits first; the override is only required to satisfy the pure-virtual contract.
struct DefaultAlwaysFailParseHandler final : RoutineHandler<AlwaysFailParse>
{
    Result<std::optional<AlwaysFailParse>> Start(std::optional<AlwaysFailParse> /*params*/) override
    {
        return std::optional<AlwaysFailParse>{std::nullopt};
    }
};

/// RoutineHandler for testing Stop and CompletionPercentage defaults.
struct DefaultRoutineHandler final : RoutineHandler<TestPayload>
{
    Result<std::optional<TestPayload>> Start(std::optional<TestPayload> /*params*/) override
    {
        return Result<std::optional<TestPayload>>(score::cpp::make_unexpected(NegativeResponseCode::SubFunctionNotSupported));
    }
};

}  // namespace

// ── RoutineHandler<T> — default implementations ───────────────────────────────

TEST(RoutineHandlerTest, StopDefaultReturnsSubFunctionNotSupported)
{
    DefaultRoutineHandler routine_handler{};
    const auto result = routine_handler.Stop(std::nullopt);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SubFunctionNotSupported);
}

TEST(RoutineHandlerTest, CompletionPercentageDefaultReturnsNullopt)
{
    const DefaultRoutineHandler routine_handler{};
    EXPECT_FALSE(routine_handler.CompletionPercentage().has_value());
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
        CapturingHandler(bool& is_called, std::uint8_t& received) : called{is_called}, received_value{received} {}
        Result<score::cpp::blank> HandleWrite(TestPayload val) override
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
        Result<score::cpp::blank> HandleWrite(AlwaysFailParse /*val*/) override
        {
            ADD_FAILURE() << "HandleWrite must not be called on parse failure";
            return {};
        }
    };

    SerializedWriteDataByIdentifier<AlwaysFailParse, NeverCalledHandler> writer{NeverCalledHandler{}};
    const ByteVector input{std::byte{0x00U}};
    const auto result = writer.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedWriteDataByIdentifierTest, WriteHandlerErrorIsPropagated)
{
    SerializedWriteDataByIdentifier<TestPayload, FailingWriteHandler> writer{FailingWriteHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = writer.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

// ── SerializedGenericDataIdentifier<T, H> ─────────────────────────────────────

TEST(SerializedGenericDataIdentifierTest, ReadReturnsSerializedBytes)
{
    SerializedGenericDataIdentifier<TestPayload, FailingWriteHandler> did{TestPayload{0xABU},
                                                                          FailingWriteHandler{}};

    const auto result = did.Read();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0], std::byte{0xABU});
}

TEST(SerializedGenericDataIdentifierTest, ReadPropagatesSerializeError)
{
    struct NoOpWriteHandler : public WriteHandler<AlwaysFailSerialize>
    {
        Result<score::cpp::blank> HandleWrite(AlwaysFailSerialize /*val*/) override { return {}; }
    };

    SerializedGenericDataIdentifier<AlwaysFailSerialize, NoOpWriteHandler> did{AlwaysFailSerialize{},
                                                                               NoOpWriteHandler{}};

    const auto result = did.Read();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedGenericDataIdentifierTest, WriteSuccessfullyParsesAndCallsHandler)
{
    bool handler_called{false};
    std::uint8_t handler_received_value{0U};

    struct CapturingHandler final : public WriteHandler<TestPayload>
    {
        bool& called;
        std::uint8_t& received_value;
        CapturingHandler(bool& is_called, std::uint8_t& received)
            : called{is_called}, received_value{received}
        {
        }
        Result<score::cpp::blank> HandleWrite(TestPayload val) override
        {
            called = true;
            received_value = val.value;
            return {};
        }
    };

    CapturingHandler handler{handler_called, handler_received_value};
    SerializedGenericDataIdentifier<TestPayload, CapturingHandler> did{TestPayload{0U},
                                                                       std::move(handler)};

    const ByteVector input{std::byte{0x55U}};
    const auto result = did.Write(ByteView{input});

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(handler_received_value, 0x55U);
}

TEST(SerializedGenericDataIdentifierTest, WriteEmptyInputReturnsIncorrectMessageLength)
{
    SerializedGenericDataIdentifier<TestPayload, FailingWriteHandler> did{TestPayload{},
                                                                          FailingWriteHandler{}};

    const ByteVector empty{};
    const auto result = did.Write(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedGenericDataIdentifierTest, WriteParseFailureDoesNotCallHandler)
{
    struct NeverCalledHandler : public WriteHandler<AlwaysFailParse>
    {
        Result<score::cpp::blank> HandleWrite(AlwaysFailParse /*val*/) override
        {
            ADD_FAILURE() << "HandleWrite must not be called on parse failure";
            return {};
        }
    };

    SerializedGenericDataIdentifier<AlwaysFailParse, NeverCalledHandler> did{AlwaysFailParse{},
                                                                             NeverCalledHandler{}};
    const ByteVector input{std::byte{0x00U}};
    const auto result = did.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedGenericDataIdentifierTest, WriteHandlerErrorIsPropagated)
{
    SerializedGenericDataIdentifier<TestPayload, FailingWriteHandler> did{TestPayload{},
                                                                          FailingWriteHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = did.Write(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

// ── SerializedRoutineControl<T, H> ────────────────────────────────────────────

TEST(SerializedRoutineControlTest, StartWithNoInputReturnsEmptyBytes)
{
    SerializedRoutineControl<TestPayload, NoReplyStartHandler> ctrl{NoReplyStartHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Start(ByteView{empty});

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(SerializedRoutineControlTest, StartWithInputParsesAndReturnsSerializedReply)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = ctrl.Start(ByteView{input});

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0], std::byte{0x42U});
}

TEST(SerializedRoutineControlTest, StartInputParseFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailParse, DefaultAlwaysFailParseHandler> ctrl{
        DefaultAlwaysFailParseHandler{}};

    const ByteVector input{std::byte{0x00U}};
    const auto result = ctrl.Start(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedRoutineControlTest, StartHandlerErrorIsPropagated)
{
    SerializedRoutineControl<TestPayload, FailingStartHandler> ctrl{FailingStartHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Start(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StartSerializeReplyFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailSerialize, SerializeFailOnStartHandler> ctrl{
        SerializeFailOnStartHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Start(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::FailurePreventsExecutionOfRequestedAction);
}

TEST(SerializedRoutineControlTest, StopWithValidInputParsesAndReturnsSerializedReply)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const ByteVector input{std::byte{0x01U}};
    const auto result = ctrl.Stop(ByteView{input});

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0], std::byte{0x43U});
}

TEST(SerializedRoutineControlTest, StopWithNoInputReturnsSerializedReply)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Stop(ByteView{empty});

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ((*result)[0], std::byte{0x43U});
}

TEST(SerializedRoutineControlTest, StopWithNoReplyPayloadReturnsEmptyBytes)
{
    SerializedRoutineControl<TestPayload, NoReplyStopHandler> ctrl{NoReplyStopHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Stop(ByteView{empty});

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(SerializedRoutineControlTest, StopInputParseFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailParse, DefaultAlwaysFailParseHandler> ctrl{
        DefaultAlwaysFailParseHandler{}};

    const ByteVector input{std::byte{0x00U}};
    const auto result = ctrl.Stop(ByteView{input});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
}

TEST(SerializedRoutineControlTest, StopHandlerErrorIsPropagated)
{
    SerializedRoutineControl<TestPayload, FailingStopHandler> ctrl{FailingStopHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Stop(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(SerializedRoutineControlTest, StopSerializeReplyFailureReturnsError)
{
    SerializedRoutineControl<AlwaysFailSerialize, SerializeFailOnStopHandler> ctrl{
        SerializeFailOnStopHandler{}};

    const ByteVector empty{};
    const auto result = ctrl.Stop(ByteView{empty});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::FailurePreventsExecutionOfRequestedAction);
}

TEST(SerializedRoutineControlTest, CompletionPercentageDelegatesToHandler)
{
    SerializedRoutineControl<TestPayload, SuccessRoutineHandler> ctrl{SuccessRoutineHandler{}};

    const auto pct = ctrl.CompletionPercentage();

    ASSERT_TRUE(pct.has_value());
    EXPECT_EQ(*pct, 75U);
}

// ── DeserializeRequest<T>() ─────────────────────────────────────────────────

TEST(DeserializeRequestTest, SuccessfulParseInvokesCallable)
{
    TestPayload received{};
    bool called{false};

    const ByteVector input{std::byte{0x99U}};
    const auto result =
        DeserializeRequest<TestPayload>(ByteView{input}, [&](TestPayload val) -> Result<score::cpp::blank>{
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
        DeserializeRequest<TestPayload>(ByteView{empty}, [&](TestPayload /*val*/) -> Result<score::cpp::blank>{
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
        DeserializeRequest<TestPayload>(ByteView{input}, [](TestPayload /*val*/) -> Result<score::cpp::blank>{
            return Result<score::cpp::blank>(score::cpp::make_unexpected(NegativeResponseCode::SecurityAccessDenied));
        });

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SecurityAccessDenied);
}

}  // namespace score::mw::diag::uds
