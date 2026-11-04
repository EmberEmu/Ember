add_library(asio_config INTERFACE)

target_compile_definitions(asio_config INTERFACE
    BOOST_ASIO_SEPARATE_COMPILATION
)

set(ASIO_IMPL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/asio_impl.cpp")

file(WRITE "${ASIO_IMPL_SOURCE}"
    "#include <boost/asio/impl/src.hpp>"
)

add_library(asio_impl STATIC "${ASIO_IMPL_SOURCE}")

target_link_libraries(asio_impl PUBLIC asio_config Boost::headers)