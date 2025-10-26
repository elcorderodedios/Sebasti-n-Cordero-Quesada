cmake_minimum_required(VERSION 3.22)
project(ProductionLineSimulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add executable
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/core/ProductionController.cpp
    src/core/ThreadManager.cpp
    src/core/WorkStation.cpp
    src/core/stations/Intake.cpp
    src/core/stations/Assembler.cpp
    src/core/stations/QualityInspection.cpp
    src/core/stations/Packaging.cpp
    src/core/stations/Shipping.cpp
    src/ipc/PipeManager.cpp
    src/ipc/SocketComm.cpp
    src/model/Product.cpp
    src/persist/Persistence.cpp
    src/logging/Logger.cpp
    src/stats/StatsAggregator.cpp
)

set(HEADERS
    src/ui/MainWindow.h
    src/core/ProductionController.h
    src/core/ThreadManager.h
    src/core/WorkStation.h
    src/core/Buffer.h
    src/core/stations/Intake.h
    src/core/stations/Assembler.h
    src/core/stations/QualityInspection.h
    src/core/stations/Packaging.h
    src/core/stations/Shipping.h
    src/ipc/PipeManager.h
    src/ipc/SocketComm.h
    src/model/Product.h
    src/persist/Persistence.h
    src/logging/Logger.h
    src/stats/StatsAggregator.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Charts)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
