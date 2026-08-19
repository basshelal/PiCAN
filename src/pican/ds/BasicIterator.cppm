export module pican.ds:BasicIterator;

export namespace pican::ds {
template<typename TP>
class BasicIterator {
private:  // fields
    TP* ptr_f;

public:  // constructor
    explicit BasicIterator(TP* ptr) : ptr_f{ptr} {
    }

public:  // lifetime
    BasicIterator(const BasicIterator& rhs) = default;

    BasicIterator(BasicIterator&& rhs) noexcept = default;

    BasicIterator&
    operator=(const BasicIterator& rhs) & = default;

    BasicIterator&
    operator=(BasicIterator&& rhs) & noexcept = default;

    ~BasicIterator() = default;

public:  // member functions
    [[nodiscard]]
    TP*
    get() const {
        return this->ptr_f;
    }

    TP&
    operator*() const {
        return *this->ptr_f;
    }

    BasicIterator&
    operator++() {
        this->ptr_f++;
        return *this;
    }

    BasicIterator&
    operator--() {
        this->ptr_f--;
        return *this;
    }

    bool
    operator==(const BasicIterator& rhs) const {
        return this->ptr_f == rhs.ptr_f;
    }

    bool
    operator!=(const BasicIterator& rhs) const {
        return this->ptr_f != rhs.ptr_f;
    }
};
}  // namespace pican::ds
