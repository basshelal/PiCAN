#include <string>

#include <catch2/catch_all.hpp>

import pican.test_utils;
import pican.core;

using pican::Result;
using pican::ScopeGuard;
using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;
using SuccessType = Tracked<std::string>;
using FailureType = Tracked<std::string>;
using ResultType = Result<SuccessType, FailureType>;

// TODO(bxh) 06-Sep-26 23:48 We need to be more exhaustive here!
bool
is_success(const ResultType& result) {
    const bool isSuccess = result.is_success();
    const bool isFailure = result.is_failure();
    CHECK(isSuccess);
    CHECK(!isFailure);
    return isSuccess && !isFailure;
}

bool
is_failure(const ResultType& result) {
    const bool isSuccess = result.is_success();
    const bool isFailure = result.is_failure();
    CHECK(!isSuccess);
    CHECK(isFailure);
    return !isSuccess && isFailure;
}

TEST_CASE("Result") {
    const std::string successData{"success"};
    const SuccessType success{successData};
    SECTION("Success by copy") {
        const ResultType result = ResultType::success_by_copy(success);
        CHECK(is_success(result));

        const SuccessType& successValue = result.success_value_or_panic();
        CHECK(successValue.data == success.data);
        CHECK(successValue.address() != success.address());

        CHECK(successValue.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValue.copyCount == 1);
        CHECK(successValue.moveCount == 0);
    }

    SECTION("Success by move") {
        ResultType result = ResultType::success_by_move(SuccessType{successData});
        CHECK(is_success(result));

        const SuccessType& successValue = result.success_value_or_panic();
        CHECK(successValue.data == successData);

        CHECK(successValue.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(successValue.copyCount == 0);
        CHECK(successValue.moveCount == 1);
    }

    SECTION("Success emplace") {
        ResultType result = ResultType::success_emplace(successData);
        CHECK(is_success(result));

        const SuccessType& successValue = result.success_value_or_panic();
        CHECK(successValue.data == successData);

        CHECK(successValue.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValue.copyCount == 0);
        CHECK(successValue.moveCount == 0);
    }

    SECTION("Owns the memory of the success value") {
        ResultType result = GENERATE_COPY(
            ResultType::success_by_copy(success), ResultType::success_by_move(SuccessType{successData}),
            ResultType::success_emplace(successData)
        );
        CHECK(is_success(result));

        SuccessType& successValue = result.success_value_or_panic();

        std::byte* resultAddress = reinterpret_cast<std::byte*>(&result);
        std::byte* successAddress = reinterpret_cast<std::byte*>(&successValue);

        CHECK(sizeof(ResultType) >= sizeof(SuccessType));
        std::byte* expectedAddress = resultAddress + (alignof(SuccessType));
        CHECK(successAddress == expectedAddress);
    }

    SECTION("Extracting success value") {
        int onMovedFromCalledCount = 0;
        SuccessType::LifetimeCallback onMovedFrom = [&](const auto&) -> void {
            onMovedFromCalledCount++;
        };
        SuccessType success{successData, {.onMoveConstructedFrom = onMovedFrom}};
        ResultType result = ResultType::success_by_copy(success);

        SuccessType extracted{std::move(result).success_value_extract_or_panic()};
        CHECK(extracted.data == successData);
        CHECK(extracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(extracted.copyCount == 0);
        CHECK(extracted.moveCount == 1);
        CHECK(onMovedFromCalledCount == 1);
    }

    SECTION("Success destructor called") {
        int destructorCalledCount = 0;
        {
            Tracked<std::string>::LifetimeCallbacks callbacks{
                .onDestructed = [&](const auto&) -> void {
                    destructorCalledCount++;
                },
            };
            CHECK(destructorCalledCount == 0);
            auto result = ResultType::success_emplace(successData, callbacks);
            CHECK(destructorCalledCount == 0);
        }
        CHECK(destructorCalledCount == 1);
    }

    SECTION("success value or panic const") {
        const ResultType result = ResultType::success_emplace(successData);

        CHECK(is_success(result));

        const SuccessType& successValueRef = result.success_value_or_panic();
        CHECK(successValueRef.data == successData);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_panic();
        CHECK(successValueCopy.data == successData);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success value or panic") {
        ResultType result = ResultType::success_emplace(successData);

        CHECK(is_success(result));

        SuccessType& successValueRef = result.success_value_or_panic();
        CHECK(successValueRef.data == successData);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_panic();
        CHECK(successValueCopy.data == successData);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success value or else const") {
        const ResultType result = ResultType::success_emplace(successData);

        CHECK(is_success(result));

        const SuccessType defaultValue{"default"};

        const SuccessType& successValueRef = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == successData);
        CHECK(successValueRef.data != defaultValue.data);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_else(defaultValue);
        CHECK(successValueCopy.data == successData);
        CHECK(successValueRef.data != defaultValue.data);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success_value_or_else_mut__success") {
        ResultType result = ResultType::success_emplace(successData);

        CHECK(is_success(result));

        SuccessType defaultValue{"default"};

        SuccessType& successValueRef = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == successData);
        CHECK(successValueRef.data != defaultValue.data);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_else(defaultValue);
        CHECK(successValueCopy.data == successData);
        CHECK(successValueRef.data != defaultValue.data);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success_value_or_else_const__failure") {
        const ResultType result = ResultType::failure_emplace("failure");

        CHECK(is_failure(result));

        const SuccessType defaultValue{"default"};

        const SuccessType& successValueRef = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == defaultValue.data);
        CHECK(successValueRef.data != successData);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == defaultValue.data);
        CHECK(successValueRef.data != successData);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success_value_or_else_mut__failure") {
        ResultType result = ResultType::failure_emplace("failure");

        CHECK(is_failure(result));

        SuccessType defaultValue{"default"};

        SuccessType& successValueRef = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == defaultValue.data);
        CHECK(successValueRef.data != successData);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_else(defaultValue);
        CHECK(successValueRef.data == defaultValue.data);
        CHECK(successValueRef.data != successData);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }

    SECTION("success_extract_value_or_panic") {
        ResultType result = ResultType::success_emplace(successData);

        CHECK(is_success(result));

        const SuccessType& successValueRef = std::move(result).success_value_extract_or_panic();

        CHECK(successValueRef.data == successData);

        CHECK(successValueRef.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(successValueRef.copyCount == 0);
        CHECK(successValueRef.moveCount == 0);

        SuccessType successValueCopy = result.success_value_or_panic();
        CHECK(successValueCopy.data == successData);

        CHECK(successValueCopy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(successValueCopy.copyCount == 1);
        CHECK(successValueCopy.moveCount == 0);
    }
}

// TODO write failure tests!
