#include <concepts>

template<typename T>
concept CopyableOnly = std::copy_constructible<T> && std::assignable_from<T&, T&> &&
                       std::assignable_from<T&, const T&> && std::assignable_from<T&, const T>;
template<typename T>
concept MovableOnly = std::movable<T>;

template<typename ...Args>
concept AllCopyableOnly = (CopyableOnly<Args> && ...);

template<typename ...Args>
concept AllMovableOnly = (MovableOnly<Args> && ...);
