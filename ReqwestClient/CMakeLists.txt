
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/include/*.h")

add_executable(ReqwestClient ${SOURCES} ${HEADERS})

target_include_directories(ReqwestClient PRIVATE
    "vendor"
    "src/include"
    "C:/Program Files/OpenSSL-Win64/include")

target_link_directories(
    ReqwestClient
    PRIVATE
    "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MT")

# Crypt32.lib (windows)
# libcrypto + libssl (openssl)
target_link_libraries(ReqwestClient PRIVATE
    Crypt32.lib
    libcrypto_static.lib
    libssl_static.lib)
