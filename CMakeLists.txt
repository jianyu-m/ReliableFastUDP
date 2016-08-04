cmake_minimum_required(VERSION 2.8)
project(ReliableFastUDP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES src/package/PackageBase.cpp
                 src/package/PackageData.cpp
                 src/package/PackageConnectionCreate.cpp
        src/package/PackageConnectionCreateR.cpp
        src/package/PackageConnectionClose.cpp
        src/package/PackageConnectionCloseR.cpp
        src/package/PackagePing.cpp
        src/ReliableSocket.cpp
        src/package/PackageAck.cpp
        src/pthread_wrapper/Mutex.cpp
        src/pthread_wrapper/ConditionVariable.cpp)

#add_executable(ReliableFastUDP ${SOURCE_FILES})
add_executable(TestServer ${SOURCE_FILES} test/test_trans.cpp)
add_executable(TestClient ${SOURCE_FILES} test/test_client.cpp)
target_link_libraries(TestServer glog)
target_link_libraries(TestClient glog)