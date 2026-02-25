#include <variant>

template <typename Variant, typename Func, std::size_t I = 0>
void for_each_type(Func&& func) {
    if constexpr (I < std::variant_size_v<Variant>) {
        using T = std::variant_alternative_t<I, Variant>;
        func.template operator()<T>();
        for_each_type<Variant, Func, I + 1>(std::forward<Func>(func));
    }
};

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};