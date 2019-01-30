#pragma once
//
// Author: Vladimir Petter
// github: https://github.com/vladp72/iffl
//
// This module contains few metaprogramming facilities that
// are not part of the STL
//
//  Helpers for detect idiom. See https://www.youtube.com/watch?v=U3jGdnRL3KI 
//  It can be used to safely deduce if an expression is valid for a type.
//  Most likley can be replaced with concepts once they are officially 
//  part of the language
//
#include <type_traits>

namespace iffl::mpl {

//
// Type that can take any number of template 
// parameters and returns void.
// It is used by detector metafunction SFINAE
//
template <typename... Whatever >
using void_t = std::void_t<Whatever>;

//
// A valid type that cannot be instantiate.
// Used by some detector metafunctions 
// as a default type.
//
struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const &) = delete;
    nonesuch const &operator=(nonesuch const &) = delete;
};

namespace details {
    //
    // defines detector metafunction
    //

    //
    // General case is selected if
    // no better specialization is found
    //
    // This metafunction has 2 return types
    //
    // 1. value_type is either std::true_type or
    // std::false_type, depending if Operation<Arguments...>
    // is well formed.
    //
    // 2. type is a type of Operation<Arguments...> or
    // a type of Default, depending if Operation<Arguments...>
    // is well formed.
    //
    // Input parameters
    //
    // 1. Default type of result "type" is Operation<Arguments...>
    // is not well formed, otherwised it is not used
    //
    // 2. AlwaysVoid is placeholder where specialization would
    // attempt to evaluate std::void_t<Operation<Arguments ...>>.
    // void_t returns type void no matter what parameters we pass
    // 
    // 3. Operation is the metafunction that we are trying to evaluate
    // for the Arguments...
    //
    // 4. Parameters for the metafunction Operation
    //
    template<typename Default,
             typename AlwaysVoid,
             template<typename... > typename Operation,
             typename... Arguments>
    struct detector {
        using value_type = std::false_type;
        using type = Default;
    };

    //
    // if Operation<Arguments...> is well formed
    // then this specialization is prefered
    // otherwise we fall-back to the default above
    //
    template<typename Default,
             template<typename... > typename Operation,
             typename... Arguments>
    struct detector<Default,
                    void_t<Operation<Arguments...>>,
                    Operation,
                    Arguments... > {
        using value_type = std::true_type;
        using type = Operation<Arguments...>;
    };

} // namespace details

//
// is_detected will be either true_type or false_type,
// depending if Operation<Arguments...> metafunction
// is well formed
//
//
template <template <typename... > typename Operation,
          typename... Arguments>
using is_detected =
        typename details::detector <nonesuch,
                                    void,
                                    Operation,
                                    Arguments...>::value_type;

//
// Technically we do not need this because is_detected already 
// returns value_type, but we will define it anyways in case
// if folks expect it
//
template <template <typename... > typename Operation,
          typename... Arguments>
using is_detected_t = is_detected<Operation, Arguments>;

template <template <typename... > typename Operation,
          typename... Arguments>
constexpr auto const is_detected_v{ is_detected<Operation, Arguments...>{} };

//
// detected_t will be either nonesuch or 
// result of metafunction Operation<Arguments...>,
// depending if Operation<Arguments...> metafunction
// is well formed
//
template <template <typename... > typename Operation,
          typename... Arguments>
using detected_t =
    typename details::detector <nonesuch,
                                void,
                                Operation,
                                Arguments...>::type;

template <template <typename... > typename Operation,
          typename... Arguments>
constexpr auto const detected_v{ detected_t<Operation, Arguments...>{} };

//
// This is a metafunction that returns
// type of Operation<Arguments...> if it is well formed,
// and otherwise Default type
//
template <typename Default,
          template <typename... > typename Operation,
          typename... Arguments>
using detected_or =
    details::detector <Default,
                       void,
                       Operation,
                       Arguments...>;

template <typename Default,
          template <typename... > typename Operation,
          typename... Arguments>
using detected_or_t = typename detected_or<Default, Operation, Arguments...>::type;

template <typename Default,
          template <typename... > typename Operation,
          typename... Arguments>
constexpr auto const detected_or_v{ typename detected_or<Default, Operation, Arguments...>::type{} };

//
// This is a metafunction that returns true when 
// type of Operation<Arguments...>, if it is well formed,
// is same as ExpectedType
//
template <typename ExpectedType,
          template <typename... > typename Operation,
          typename... Arguments>
using is_detected_exact =
    std::is_same<ExpectedType,
                 detected_t<Operation,
                            Arguments... >>;

template <typename ExpectedType,
          template <typename... > typename Operation,
          typename... Arguments>
constexpr auto const is_detected_exact_v{ is_detected_exact<ExpectedType, Operation, Arguments...>::value{} };

//
// This is a metafunction that returns true when 
// type of Operation<Arguments...>, if it is well formed,
// is vonvertable as ExpectedType
//
template <typename ConvertableToType,
          template <typename... > typename Operation,
          typename... Arguments>
using is_detected_convertable =
            std::is_convertible<detected_t<Operation,
                                           Arguments... >,
                                ConvertableToType>;

template <typename ConvertableToType,
          template <typename... > typename Operation,
          typename... Arguments>
constexpr auto const is_detected_convertable_v{ is_detected_convertable<ConvertableToType, Operation, Arguments...>::value{} };

} //namespace iffl::mpl;

////
//// To demonstrate use cases of detect idiom we will define 2 classes
////
//// This class has only foo, and does not have bar
////
//class test1 {
//public:
//    long long foo() {
//        printf("called test1::foo()\n");
//        return 111;
//    }
//};
////
//// This class has only bar, and does not have foo
//// it also defines a nested typedef for difference_type
////
//class test2 {
//public:
//    long long boo() {
//        printf("called test2::boo()\n");
//        return 112;
//    }
//
//    using difference_type = char;
//};
//
////
//// A metafunction that we will use to detect if
//// T{}.foo() is a valid expression 
////
//template <typename T>
//using has_foo = decltype(std::declval<T &>().foo());
//
////
//// A metafunction that we will use to detect if
//// T{}.boo() is a valid expression 
////
//template <typename T>
//using has_boo = decltype(std::declval<T &>().boo());
//
////
//// Use Case 1: 
////
//// If T{}.foo() is valid then call it
//// If T{}.bar() is valid then call it instead.
////
//// We are using SFINAE to eliminate candidates that are not possible
//// We are using dependent template parameters on the input types, which turns of template type deduction,
//// and because of that we split SFINAE portion to *_impl, and we rely on call_if_can
//// to deduce parameter type, and explicitely pass it to the *_impl.
////
//template <typename T>
//auto call_if_can_impl( std::enable_if_t <mpl::is_detected_v<has_foo, T>, T &> t) {
//    printf("can call foo\n");
//    return t.foo();
//}
//
//template <typename T>
//auto call_if_can_impl( std::enable_if_t <mpl::is_detected_v<has_boo, T>, T &> t) {
//    printf("can call boo\n");
//    return t.boo();
//}
//
//template <typename T>
//auto call_if_can( T & t ) {
//    return call_if_can_impl<T>(t);
//}
//
////
//// Use Case 2:
////
//// If T{}.foo() is valid then call it
//// If T{}.bar() is valid then call it instead.
//// It is the same as Use Case 1, but in this case we do 
//// SFINAE on unnamed default template parameter.
//// in this case type of template parameters deduces fine, but
//// compiler complains that two functions have same input and out parameters
//// so we need to add an artificial default initialized parameter 
//// std::integral_constant<int, ?> to make two signatures slightly different.
////
//template <typename T,
//          typename = std::enable_if_t <mpl::is_detected_v<has_foo, T>, void>>
//auto call_if_can2(T & t, std::integral_constant<int, 1> const = {}) {
//    printf("can call 2 foo\n");
//    return t.foo();
//}
//
//template <typename T,
//          typename = std::enable_if_t <mpl::is_detected_v<has_boo, T>, void>>
//auto call_if_can2(T & t, std::integral_constant<int, 2> const = {}) {
//    printf("can call 2 boo\n");
//    return t.boo();
//}
//
////
//// Use Case 3:
////
//// If T::differentce type is defined then use it, otherwise 
//// use std::ptrdiff_t.
////
//template<class T>
//using diff_t = typename T::difference_type;
// 
//template <class T>
//using difference_type = mpl::detected_or_t<std::ptrdiff_t, diff_t, T>;
//
//static_assert(std::is_same_v<std::ptrdiff_t, difference_type<test1>>, "test1 does not have difference_type");
//static_assert(std::is_same_v<char, difference_type<test2>>, "test2 overwrites difference_type ");
//
//void test_detect() {
//
//    test1 t1;
//    test2 t2;
//    //
//    // Use Case 4:
//    //
//    // Use for static asserts
//    // Assert that test1{}.foo() is valid, and test1{}.boo() is not valid
//    //
//    static_assert(mpl::is_detected_v<has_foo, test1>, "test1 must have foo()");
//    static_assert(!mpl::is_detected_v<has_boo, test1>, "test1 must not have boo()");
//    //
//    // Use Case 5:
//    //
//    // Use if constexpr to choose one of code branches.
//    // In many cases it is easier comparing to SFINAE on functions,
//    // but it requires C++17
//    //
//    if constexpr (mpl::is_detected<has_foo, test1>{}) {
//        printf("test1 has foo\n");
//        t1.foo();
//    } else {
//        printf("test1 has no foo\n");
//    }
//
//    if constexpr (mpl::is_detected<has_boo, test1>{}) {
//        printf("test1 has boo\n");
//    } else {
//        printf("test1 has no boo, calling foo instead\n");
//        t1.foo();
//    }
//    //
//    // Test Use Case 1
//    //
//    call_if_can(t1);
//    call_if_can(t2);
//    //
//    // Test Use Case 2
//    //
//    call_if_can2(t1);
//    call_if_can2(t2);
//}
