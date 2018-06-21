#include "v8pp/config.hpp"

#if !V8PP_HEADER_ONLY
#include "v8pp/convert.hpp"

namespace v8pp {

template struct convert<std::string>;
template struct convert<char const*>;
#ifdef _WIN32
template struct convert<std::wstring>;
template struct convert<wchar_t const*>;
#endif
template struct convert<bool>;

template struct convert<char>;
template struct convert<signed char>;
template struct convert<unsigned char>;

template struct convert<short>;
template struct convert<unsigned short>;

template struct convert<int>;
template struct convert<unsigned int>;

template struct convert<long>;
template struct convert<unsigned long>;

template struct convert<long long>;
template struct convert<unsigned long long>;

template struct convert<float>;
template struct convert<double>;
template struct convert<long double>;

} // v8pp
#endif
