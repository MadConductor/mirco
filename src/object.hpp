#include <memory>

namespace object
{
    template<typename T>
    std::unique_ptr<T> clone(const T& object)
    {
        using base_type = typename T::base_type;
        static_assert(std::is_base_of<base_type, T>::value, "T object has to derived from T::base_type");
        auto ptr = static_cast<const base_type&>(object).clone();
        return std::unique_ptr<T>(static_cast<T*>(ptr));
    }

    template<typename T>
    auto clone(T* object) -> decltype(clone(*object))
    {
        return clone(*object);
    }

    template<typename T>
    struct cloneable
    {
        using base_type = T;

        virtual ~cloneable() = default;
    public:
        virtual T* clone() const = 0;

        template <typename X>
        friend std::unique_ptr<X> object::clone(const X&);
    };
}
