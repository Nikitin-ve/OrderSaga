cmake_minimum_required(VERSION 3.14)
project(OrderSaga VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(LIB_SOURCES
    src/models/money.cpp
    src/models/user.cpp
    src/models/product.cpp
    src/models/order.cpp
    src/services/discount_service.cpp
    src/services/inventory_service.cpp
    src/services/billing_service.cpp
    src/services/notification_service.cpp
    src/saga/order_saga_orchestrator.cpp
    src/services/order_service.cpp
)

add_library(order_saga_lib ${LIB_SOURCES})
target_include_directories(order_saga_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(order_saga src/main.cpp)
target_link_libraries(order_saga order_saga_lib)

enable_testing()

add_executable(tests
    tests/test_main.cpp
    tests/test_discount_service.cpp
    tests/test_inventory_service.cpp
    tests/test_billing_service.cpp
    tests/test_saga.cpp
)
target_link_libraries(tests order_saga_lib)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_test(NAME AllTests COMMAND tests)
