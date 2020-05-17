//
// Created by eric on 5/13/20.
//

#ifndef TGCLIC_UTILS_H
#define TGCLIC_UTILS_H


#include <memory>

namespace detail {
    template <class... Fs>
    struct overload;

    template <class F>
    struct overload<F> : public F {
        explicit overload(F f) : F(f) {
        }
    };
    template <class F, class... Fs>
    struct overload<F, Fs...>
            : public overload<F>
                    , overload<Fs...> {

        overload(F f, Fs... fs) : overload<F>(f), overload<Fs...>(fs...) {
        }

        using overload<F>::operator();
        using overload<Fs...>::operator();
    };
}  // namespace detail

template <class... F>
auto overloaded(F... f) {
    return detail::overload<F...>(f...);
}

template <typename T>
class singleton
{
private:
    static std::unique_ptr<T> p;

public:
    static T& instance(){
        return *p;
    }
};

template <typename T>
std::unique_ptr<T> singleton<T>::p = std::make_unique<T>();

#endif //TGCLIC_UTILS_H
