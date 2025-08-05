#ifndef OPTIMISED_BOOST_HEADER_HPP
#define OPTIMISED_BOOST_HEADER_HPP

#pragma push_macro("VOID")
#pragma push_macro("DELETE")
#pragma push_macro("IN")
#pragma push_macro("OUT")
#undef VOID
#undef DELETE
#undef IN
#undef OUT

#pragma warning(push)
#pragma warning(disable : 4820 4514 4365)
#include <boost/circular_buffer.hpp>
#pragma warning(pop)

#pragma pop_macro("OUT")
#pragma pop_macro("IN")
#pragma pop_macro("DELETE")
#pragma pop_macro("VOID")

#endif  // !OPTIMISED_BOOST_HEADER_HPP