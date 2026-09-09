#pragma once

// Convenience Macros for pican::dbc contracts
// Note: You must `import dbc;` in any source file that uses these macros.

#define PICAN_DBC_CONCAT_IMPL(x, y) x##y
#define PICAN_DBC_CONCAT(x, y) PICAN_DBC_CONCAT_IMPL(x, y)

#define PRECONDITION(cond, ...) pican::contracts::precondition(cond, ##__VA_ARGS__)
#define POSTCONDITION(...)                                                            \
    pican::contracts::PostconditionChecker PICAN_DBC_CONCAT(_pican_post_, __LINE__) { \
        __VA_ARGS__                                                                   \
    }
#define CHEK_CLASS_INVARIANTS(obj)                                                     \
    pican::contracts::InvariantChecker PICAN_DBC_CONCAT(_pican_class_inv_, __LINE__) { \
        obj                                                                            \
    }
#define LOOP_INVARIANT(...)                                                               \
    pican::contracts::LoopInvariantChecker PICAN_DBC_CONCAT(_pican_loop_inv_, __LINE__) { \
        __VA_ARGS__                                                                       \
    }
