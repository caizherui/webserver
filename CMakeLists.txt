cmake_minimum_required(VERSION 3.10)  # 设置所需的 CMake 最低版本

set(PROJECT_NAME OptWebServer)

# 项目信息
project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(${PROJECT_NAME} src/main.cpp src/config/config.cpp src/http/http_conn.cpp src/webserver/webserver.cpp src/CGImysql/sql_connection_pool.cpp src/log/log.cpp src/timer/lst_timer.cpp)

# 查找并包含 pthread 库
find_package(Threads REQUIRED)
if(Threads_FOUND)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

# 查找并包含 MySQL 客户端库
# 尝试查找 MySQL 客户端库
find_library(MYSQL_CLIENT_LIBRARIES NAMES mysqlclient PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu/)
if(MYSQL_CLIENT_LIBRARIES)
    message(STATUS "Found MySQL client libraries: ${MYSQL_CLIENT_LIBRARIES}")
    target_link_libraries(${PROJECT_NAME} ${MYSQL_CLIENT_LIBRARIES})

    # 包含 MySQL 头文件路径
    include_directories(/usr/include/mysql /usr/local/include/mysql)
else()
    message(FATAL_ERROR "MySQL client libraries not found. Please install libmysqlclient-dev.")
endif()