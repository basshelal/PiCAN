#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include "pican/Result.hpp"
#include "pican/Utils.hpp"
#include "pican/memory/Block.hpp"
#include "pican/memory/Utils.hpp"

namespace pican::memory {
template<typename TP>
class Pool {
private:  // constants
    static constexpr SizeBytes TP_SIZE = sizeof(TP);
    static constexpr Alignment TP_ALIGNMENT = alignof(TP);

private:  // types
    using Count = std::size_t;
    using Index = std::size_t;

    union Node {
    public:  // fields
        TP obj_f;
        Node* nextFreeNode_f;

    public:  // constructors
        Node() {
        }

        ~Node() {
        }
    };

    static_assert(sizeof(Node) == TP_SIZE);
    static_assert(alignof(Node) == TP_ALIGNMENT);

public:  // fields
    Block block_f;
    Count capacity_f;
    Count usedCount_f;
    Node* nextFreeNode_f;

public:  // constructors
    explicit Pool(const Block& block) :
        block_f{block}, capacity_f{block.size_bytes() / TP_SIZE}, usedCount_f{0}, nextFreeNode_f{nullptr} {
        this->nextFreeNode_f = block.ptr_at_offset<Node>(0);
        for (Index i = 0; i < this->capacity_f; ++i) {
            Node* node = block.ptr_at_offset<Node>(i * TP_SIZE);
            if (i < this->capacity_f - 1) {
                node->nextFreeNode_f = block.ptr_at_offset<Node>((i + 1) * TP_SIZE);
            } else {
                node->nextFreeNode_f = nullptr;
            }
        }
    }

public:  // copy-control
    Pool(const Pool& rhs) = delete;

    Pool(Pool&& rhs) noexcept = delete;

    Pool&
    operator=(const Pool&) & = delete;

    Pool&
    operator=(Pool&&) & noexcept = delete;

    ~Pool() = default;

public:  // functions
    [[nodiscard]]
    Count
    capacity() const& {
        return this->capacity_f;
    }

    [[nodiscard]]
    Count
    used_count() const& {
        return this->usedCount_f;
    }

    [[nodiscard]]
    Count
    available_count() const& {
        return this->capacity_f - this->usedCount_f;
    }

    [[nodiscard]]
    bool
    is_full() const& {
        return this->available_count() == 0;
    }

    [[nodiscard]]
    TP*
    get_ptr_uninitialized() & {
        if (this->is_full()) {
            return nullptr;
        }

        Node* nextFree = this->nextFreeNode_f;
        if (nextFree == nullptr) {
            return nullptr;
        }

        Node* newNextFree = nextFree->nextFreeNode_f;
        this->nextFreeNode_f = newNextFree;

        ++this->usedCount_f;
        return reinterpret_cast<TP*>(nextFree);
    }

    template<typename... Args>
    [[nodiscard]]
    TP*
    get_ptr_initialized(Args&&... args) & {
        TP* ptr = this->get_ptr_uninitialized();
        if (ptr == nullptr) {
            return nullptr;
        }

        return pican::memory::construct_at(ptr, std::forward<Args>(args)...);
    }

    void
    free(TP* obj) {
        if (!this->is_our_obj(obj)) {
            return;
        }
        Node* node = reinterpret_cast<Node*>(obj);
        node->nextFreeNode_f = this->nextFreeNode_f;
        this->nextFreeNode_f = node;
        --this->usedCount_f;
    }

    [[nodiscard]]
    bool
    is_our_obj(TP* obj) const& {
        if (obj == nullptr) {
            return false;
        }
        const Address address = pican::memory::ptr_to_address(obj);

        if (address > this->block_f.end_address() || address < this->block_f.address()) {
            return false;
        }
        if (!pican::memory::address_is_aligned(address, TP_ALIGNMENT)) {
            return false;
        }

        return true;
    }
};

}  // namespace pican::memory
