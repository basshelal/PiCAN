#define CONTRACTS_ENABLED 1

#include "pican/Contracts.hpp"
#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME Contracts

class ContractTester {
public:  // fields
    std::string* ptr_f;
    mutable std::uint64_t invariantCalled_f = 0;

    CONTRACTS_DEFINE_CLASS_INVARIANTS(
        {
            CONTRACTS_ASSERT(this->ptr_f != nullptr);
            ++this->invariantCalled_f;
        })

public:  // constructors
    explicit ContractTester(std::string* string) : ptr_f{string} {
        CONTRACTS_ASSERT_CLASS_INVARIANTS();
    }

public:  // functions
    // set functions
    void
    set_with_precondition(std::string* ptr) {
        CONTRACTS_PRECONDITION(ptr != nullptr);

        this->ptr_f = ptr;
    }

    void
    set_with_invariant(std::string* ptr) {
        CONTRACTS_ASSERT_CLASS_INVARIANTS();

        this->ptr_f = ptr;
    }

    void
    set_with_precondition_and_invariant(std::string* ptr) {
        CONTRACTS_PRECONDITION(ptr != nullptr);
        CONTRACTS_ASSERT_CLASS_INVARIANTS();
        CONTRACTS_POSTCONDITION(this->ptr_f == ptr);

        this->ptr_f = ptr;
    }

    std::string*
    set_data(const char* data) {
        CONTRACTS_PRECONDITION(::strlen(data) > 0);
        CONTRACTS_ASSERT_CLASS_INVARIANTS();

        std::string* ret;
        CONTRACTS_POSTCONDITION(ret == this->ptr_f);

        *this->ptr_f = data;
        ret = this->ptr_f;

        return ret;
    }
};

static std::uint64_t contractViolated_s = 0;

TEST(setup) {
    ASSERT_EQUAL(1, CONTRACTS_ENABLED);
    bool isEnabled = false;
    CONTRACTS_CREATE_CODE_BLOCK({ isEnabled = true; });
    ASSERT_TRUE(isEnabled);
    pican::contracts::violationHandler_g = [](const char*) -> void {
        ++contractViolated_s;
    };
}

TEST(constructor_conforming) {
    contractViolated_s = 0;

    std::string str{"my string"};
    ContractTester obj{&str};

    ASSERT_EQUAL(2, obj.invariantCalled_f);
    ASSERT_EQUAL(0, contractViolated_s);
    ASSERT_EQUAL(&str, obj.ptr_f);
}

TEST(constructor_violated) {
    contractViolated_s = 0;
    ContractTester wrapper{nullptr};

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(2, contractViolated_s);  // 2 asserts in invariant, invariant called 2 times
    ASSERT_EQUAL(nullptr, wrapper.ptr_f);
}

TEST(set_with_precondition) {
    contractViolated_s = 0;
    std::string data{"my string"};
    ContractTester wrapper{&data};

    wrapper.invariantCalled_f = 0;  // reset

    std::string newData{"my new string"};
    wrapper.set_with_precondition(&newData);

    ASSERT_EQUAL(0, wrapper.invariantCalled_f);
    ASSERT_EQUAL(0, contractViolated_s);
    ASSERT_EQUAL(&newData, wrapper.ptr_f);

    wrapper.set_with_precondition(nullptr);

    ASSERT_EQUAL(0, wrapper.invariantCalled_f);
    ASSERT_EQUAL(1, contractViolated_s);
    ASSERT_EQUAL(nullptr, wrapper.ptr_f);
}

TEST(set_with_invariant) {
    contractViolated_s = 0;
    std::string data{"my string"};
    ContractTester wrapper{&data};

    wrapper.invariantCalled_f = 0;  // reset

    std::string newData{"my new string"};
    wrapper.set_with_invariant(&newData);

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(0, contractViolated_s);
    ASSERT_EQUAL(&newData, wrapper.ptr_f);

    wrapper.invariantCalled_f = 0;  // reset

    wrapper.set_with_invariant(nullptr);

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(1, contractViolated_s);  // only once because was correct before function call but not after it
    ASSERT_EQUAL(nullptr, wrapper.ptr_f);
}

TEST(set_with_precondition_and_invariant) {
    contractViolated_s = 0;
    std::string data{"my string"};
    ContractTester wrapper{&data};

    wrapper.invariantCalled_f = 0;  // reset

    std::string newData{"my new string"};
    wrapper.set_with_precondition_and_invariant(&newData);

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(0, contractViolated_s);
    ASSERT_EQUAL(&newData, wrapper.ptr_f);

    wrapper.invariantCalled_f = 0;  // reset

    wrapper.set_with_precondition_and_invariant(nullptr);

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(2, contractViolated_s);  // once from precondition, once from failing invariant after function
    ASSERT_EQUAL(nullptr, wrapper.ptr_f);
}

TEST(set_data) {
    contractViolated_s = 0;
    std::string str{"my string"};
    ContractTester wrapper{&str};

    wrapper.invariantCalled_f = 0;  // reset

    const char* newData = "my new string";
    std::string* newStr = wrapper.set_data(newData);

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(0, contractViolated_s);
    ASSERT_EQUAL(newStr, wrapper.ptr_f);

    wrapper.invariantCalled_f = 0;  // reset

    newStr = wrapper.set_data("");

    ASSERT_EQUAL(2, wrapper.invariantCalled_f);
    ASSERT_EQUAL(1, contractViolated_s);  // once from precondition
    ASSERT_EQUAL("", *wrapper.ptr_f);
}

TEST(teardown) {
    pican::contracts::violationHandler_g = pican::contracts::defaultViolationHandler_g;
}
