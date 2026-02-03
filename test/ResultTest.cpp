#include <string>

#include "pican/Result.hpp"
#include "pican/Utils.hpp"
#include "test/TestUtils.hpp"
#include "test/Tracked.hpp"

#define TEST_SUITE_NAME Result

#define ASSERT_RESULT_IS_SUCCESS(result) \
    ASSERT_TRUE(result.is_success());    \
    ASSERT_FALSE(result.is_failure())

#define ASSERT_RESULT_IS_FAILURE(result) \
    ASSERT_FALSE(result.is_success());   \
    ASSERT_TRUE(result.is_failure())

using SuccessType = Tracked<std::string>;
using FailureType = Tracked<std::string>;
using ResultType = pican::Result<SuccessType, FailureType>;

TEST(success_by_copy) {
    const SuccessType success{"success"};
    const ResultType result = ResultType::success_by_copy(success);

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType& successValue = result.success_value_or_panic();

    ASSERT_EQUAL(success.data, successValue.data);
    ASSERT_NOT_EQUAL(success.address(), successValue.address());

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValue.lastOperation);
    ASSERT_EQUAL(1, successValue.copyCount);
    ASSERT_EQUAL(0, successValue.moveCount);
}

TEST(success_by_move) {
    const std::string successData{"success"};
    ResultType result = ResultType::success_by_move(SuccessType{successData});

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType& successValue = result.success_value_or_panic();

    ASSERT_EQUAL(successData, successValue.data);

    ASSERT_EQUAL(LifetimeOperation::MOVE_CONSTRUCTOR, successValue.lastOperation);
    ASSERT_EQUAL(0, successValue.copyCount);
    ASSERT_EQUAL(1, successValue.moveCount);
}

TEST(success_emplace) {
    const std::string successData{"success"};
    ResultType result = ResultType::success_emplace(successData);

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType& successValue = result.success_value_or_panic();

    ASSERT_EQUAL(successData, successValue.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValue.lastOperation);
    ASSERT_EQUAL(0, successValue.copyCount);
    ASSERT_EQUAL(0, successValue.moveCount);
}

TEST(ownership_of_success) {
    ResultType result = ResultType::success_emplace("success");

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType& successValue = result.success_value_or_panic();

    const ResultType* resultAddress = &result;
    const SuccessType* successAddress = &successValue;

    const void* expectedAddress = ((char*) resultAddress) + (alignof(SuccessType));
    ASSERT_EQUAL(expectedAddress, successAddress);
}

TEST(ownership_of_failure) {
    ResultType result = ResultType::failure_emplace("failure");

    ASSERT_RESULT_IS_FAILURE(result);

    const FailureType& failureValue = result.failure_value_or_panic();

    const ResultType* resultAddress = &result;
    const FailureType* failureAddress = &failureValue;

    const void* expectedAddress = ((char*) resultAddress) + (alignof(FailureType));
    ASSERT_EQUAL(expectedAddress, failureAddress);
}

TEST(success_destructor_called) {
    int destructorCalledCount = 0;
    {
        std::string str{"success"};
        std::shared_ptr<std::string> ptr{&str, [&destructorCalledCount](auto) -> void {
                                             ++destructorCalledCount;
                                         }};
        auto result = pican::Result<decltype(ptr), int>::success_by_move(std::move(ptr));
    }
    ASSERT_EQUAL(1, destructorCalledCount);
}

TEST(failure_destructor_called) {
    int destructorCalledCount = 0;
    {
        std::string str{"failure"};
        std::shared_ptr<std::string> ptr{&str, [&destructorCalledCount](auto) -> void {
                                             ++destructorCalledCount;
                                         }};
        auto result = pican::Result<int, decltype(ptr)>::failure_by_move(std::move(ptr));
    }
    ASSERT_EQUAL(1, destructorCalledCount);
}

TEST(success_value_or_panic_const) {
    const std::string successData{"success"};
    const ResultType result = ResultType::success_emplace(successData);

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType& successValueRef = result.success_value_or_panic();
    ASSERT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_panic();
    ASSERT_EQUAL(successData, successValueCopy.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

TEST(success_value_or_panic_mut) {
    const std::string successData{"success"};
    ResultType result = ResultType::success_emplace(successData);

    ASSERT_RESULT_IS_SUCCESS(result);

    SuccessType& successValueRef = result.success_value_or_panic();
    ASSERT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_panic();
    ASSERT_EQUAL(successData, successValueCopy.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

TEST(success_value_or_else_const__success) {
    const std::string successData{"success"};
    const ResultType result = ResultType::success_emplace(successData);

    ASSERT_RESULT_IS_SUCCESS(result);

    const SuccessType defaultValue{"default"};

    const SuccessType& successValueRef = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(successData, successValueRef.data);
    ASSERT_NOT_EQUAL(defaultValue.data, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(successData, successValueCopy.data);
    ASSERT_NOT_EQUAL(defaultValue.data, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

TEST(success_value_or_else_mut__success) {
    const std::string successData{"success"};
    ResultType result = ResultType::success_emplace(successData);

    ASSERT_RESULT_IS_SUCCESS(result);

    SuccessType defaultValue{"default"};

    SuccessType& successValueRef = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(successData, successValueRef.data);
    ASSERT_NOT_EQUAL(defaultValue.data, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(successData, successValueCopy.data);
    ASSERT_NOT_EQUAL(defaultValue.data, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

TEST(success_value_or_else_const__failure) {
    const std::string successData{"success"};
    const ResultType result = ResultType::failure_emplace("failure");

    ASSERT_RESULT_IS_FAILURE(result);

    const SuccessType defaultValue{"default"};

    const SuccessType& successValueRef = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(defaultValue.data, successValueRef.data);
    ASSERT_NOT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(defaultValue.data, successValueRef.data);
    ASSERT_NOT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

TEST(success_value_or_else_mut__failure) {
    const std::string successData{"success"};
    ResultType result = ResultType::failure_emplace("failure");

    ASSERT_RESULT_IS_FAILURE(result);

    SuccessType defaultValue{"default"};

    SuccessType& successValueRef = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(defaultValue.data, successValueRef.data);
    ASSERT_NOT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, successValueRef.lastOperation);
    ASSERT_EQUAL(0, successValueRef.copyCount);
    ASSERT_EQUAL(0, successValueRef.moveCount);

    SuccessType successValueCopy = result.success_value_or_else(defaultValue);
    ASSERT_EQUAL(defaultValue.data, successValueRef.data);
    ASSERT_NOT_EQUAL(successData, successValueRef.data);

    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, successValueCopy.lastOperation);
    ASSERT_EQUAL(1, successValueCopy.copyCount);
    ASSERT_EQUAL(0, successValueCopy.moveCount);
}

// TODO write failure tests!
