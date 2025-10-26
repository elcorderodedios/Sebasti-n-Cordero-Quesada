# Automated Home-Appliance Production Line Simulator

**Course**: IF-4001 Operating Systems (Group 21)  
**Platform**: Ubuntu Desktop (Linux)  
**Technology Stack**: C++20, Qt 6, CMake  
**Delivery Date**: November 17, 2025

## Table of Contents
- [Project Overview](#project-overview)
- [Architecture](#architecture)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [API Documentation](#api-documentation)
- [Testing](#testing)
- [Contributing](#contributing)
- [Known Issues](#known-issues)
- [Defense Presentation](#defense-presentation)

## Project Overview

The Automated Home-Appliance Production Line Simulator is a comprehensive graphical application that simulates an automated manufacturing production line. The system demonstrates advanced operating systems concepts including thread synchronization, inter-process communication, and concurrent programming using Qt 6 and modern C++.

### Key Objectives
- Simulate a realistic production line with multiple workstations
- Demonstrate thread-safe operations using mutexes, semaphores, and condition variables
- Provide real-time visualization and monitoring capabilities  
- Implement robust persistence and state management
- Support both threaded and process-based execution modes

### System Components
1. **5+ Workstations**: Intake, Assembler, Quality Inspection, Packaging, Shipping
2. **Thread-Safe Buffers**: Bounded producer-consumer queues with back-pressure
3. **GUI Interface**: Real-time visualization, controls, and monitoring
4. **Persistence Layer**: JSON-based state saving/loading
5. **Maintenance Threads**: Background cleanup, logging, and statistics
6. **IPC Support**: Optional inter-process communication via pipes/sockets

## Architecture

### Core Classes

#### Product (`model/Product.h`)
Represents items flowing through the production line with state tracking and traceability.

```cpp
class Product {
    ProductType m_type;           // Washer, Dryer, Refrigerator, etc.
    ProductState m_currentState;  // Created, AtIntake, AtAssembler, etc.
    QString m_id;                 // Unique identifier
    QStringList m_trace;          // Processing history
};
```

#### WorkStation (`core/WorkStation.h`)
Abstract base class for all production stations with state management and threading.

```cpp
class WorkStation : public QThread {
    StationState m_state;         // Idle, Running, Paused, Blocked, etc.
    Buffer<Product> m_inputBuffer; // Thread-safe input queue
    Buffer<Product> m_outputBuffer; // Thread-safe output queue
    
    virtual bool processProduct(Product* product) = 0;
};
```

#### Buffer<T> (`core/Buffer.h`)
Thread-safe bounded queue implementation using semaphores and mutexes.

```cpp
template<typename T>
class Buffer {
    QQueue<T> m_queue;
    QSemaphore m_spacesAvailable;
    QSemaphore m_itemsAvailable;
    QMutex m_mutex;
};
```

#### ProductionController (`core/ProductionController.h`)
Central coordinator managing the entire production line lifecycle.

```cpp
class ProductionController : public QObject {
    // Station instances and buffer connections
    // Production control (start/pause/stop/reset)
    // Statistics aggregation and event handling
};
```

### Station Implementations

1. **Intake Station**: Generates new products and feeds them into the line
2. **Assembler Station**: Performs multi-step assembly with configurable failure rates
3. **Quality Inspection**: Tests products and can send items back for rework
4. **Packaging Station**: Packages finished products with material specifications
5. **Shipping Station**: Final processing and dispatch with tracking generation

### Synchronization Mechanisms

- **QMutex/std::mutex**: Protect shared data structures
- **QSemaphore**: Implement bounded buffer semantics
- **QWaitCondition**: Enable thread pausing/resuming
- **Qt Signals/Slots**: Cross-thread communication for UI updates
- **QAtomicInt**: Lock-free status flags

## Features

### Functional Requirements ✓

#### Production Line Simulation
- **Multi-Station Pipeline**: 5 distinct processing stations
- **Individual Station Control**: Start, pause, stop each station independently
- **Automatic Flow Control**: Products flow automatically between stations
- **Quality Control**: Configurable failure rates and rework loops
- **Real-time Processing**: Live updates of product movement and station states

#### Synchronization & Communication
- **Thread-Safe Operations**: All buffer operations use proper locking
- **Back-pressure Handling**: Stations block when downstream buffers are full
- **Cooperative Threading**: Clean shutdown and pause/resume functionality
- **Signal-Slot Communication**: UI updates without blocking worker threads
- **Optional IPC Mode**: Process-based execution with pipe/socket communication

#### User Interface
- **Global Controls**: Start All, Pause All, Stop All, Reset operations
- **Station Monitoring**: Real-time status, queue depth, throughput display
- **Live Activity Log**: Filterable log with multiple severity levels
- **Metrics Dashboard**: Throughput charts, utilization graphs, performance metrics
- **Process/Thread View**: Real-time list of all active processes and threads
- **Settings Panel**: Configurable processing times, buffer sizes, failure rates

#### Persistence System
- **State Management**: Complete simulation state save/restore
- **JSON Format**: Human-readable configuration and state files
- **Auto-save**: Periodic and event-driven state persistence
- **Crash Recovery**: Automatic state restoration on application restart
- **Configuration Profiles**: Multiple saved configurations

#### Maintenance Operations
- **Background Cleanup**: `GeneralCleanThreads` - periodic resource cleanup
- **System Logging**: `GeneralLogs` - continuous event logging to file and UI
- **Statistics Aggregation**: `GeneralStats` - real-time metrics collection and charting

### Non-Functional Requirements ✓

#### Robustness
- **No Data Races**: All shared data protected by appropriate synchronization
- **Clean Shutdown**: Graceful thread termination with resource cleanup
- **Error Recovery**: Automatic recovery from minor errors and exceptions
- **Resource Management**: RAII-based memory and handle management

#### Performance
- **Responsive UI**: Non-blocking interface with background processing
- **Configurable Load**: Adjustable production rates and processing times
- **Scalable Design**: Easy addition of new stations and features
- **Memory Efficient**: Bounded queues prevent unbounded memory growth

#### Usability
- **Intuitive Controls**: Clear visual feedback and logical workflow
- **Real-time Feedback**: Immediate response to user actions
- **Comprehensive Logging**: Detailed event tracking for debugging
- **Persistent Settings**: User preferences saved between sessions

## Installation

### Prerequisites
- Ubuntu Desktop 20.04+ (tested on Ubuntu 22.04)
- Qt 6.2 or later with development packages
- CMake 3.22 or later
- GCC 11+ or Clang 12+ (C++20 support required)
- Git (for source code management)

### System Dependencies
```bash
# Install Qt 6 development packages
sudo apt update
sudo apt install -y qt6-base-dev qt6-charts-dev qt6-tools-dev

# Install build tools
sudo apt install -y cmake build-essential git

# Install additional Qt components (if needed)
sudo apt install -y qt6-declarative-dev libqt6core5compat6-dev
```

### Build Instructions

1. **Clone the Repository**
```bash
git clone <repository-url>
cd ProductionLineSimulator
```

2. **Create Build Directory**
```bash
mkdir -p build
cd build
```

3. **Configure with CMake**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

4. **Build the Application**
```bash
cmake --build . -j$(nproc)
```

5. **Run the Application**
```bash
./ProductionLineSimulator
```

### Alternative Build (Using Scripts)
```bash
# Make scripts executable
chmod +x scripts/*.sh

# Build and run
./scripts/run.sh
```

## Usage

### Getting Started

1. **Launch Application**
   ```bash
   ./ProductionLineSimulator
   ```

2. **Initial Setup**
   - Application creates configuration directory: `~/.prodline/`
   - Default settings are loaded automatically
   - Log files are stored in `~/.prodline/logs/`

3. **Start Production**
   - Click "Start All" to begin production line
   - Monitor station status and product flow
   - Watch real-time metrics and logs

### User Interface Overview

#### Main Window Layout
- **Top Toolbar**: Global production controls (Start/Pause/Stop/Reset)
- **Central Panel**: Horizontal production line with station cards
- **Dock Panels**: Logs, Metrics, Threads, Settings (draggable)
- **Status Bar**: Overall status, counters, elapsed time

#### Station Cards
Each station displays:
- Current state (Idle/Running/Paused/Blocked/Error)
- Active product being processed
- Input queue depth and capacity
- Real-time throughput (items/minute)
- Individual control buttons

#### Control Operations

**Global Controls:**
- **Start All**: Begins all stations simultaneously
- **Pause All**: Pauses all stations (preserves state)
- **Resume All**: Resumes paused stations
- **Stop All**: Graceful shutdown of all stations
- **Reset**: Clears all queues and resets statistics

**Individual Station Controls:**
- **Start**: Activate individual station
- **Pause**: Temporarily halt station processing
- **Stop**: Shutdown specific station

### Configuration

#### Settings Panel
Access via toolbar button or dock panel:

**Production Settings:**
- **Mode**: Threads Only vs Processes+IPC
- **Buffer Capacity**: Queue size between stations (1-100)
- **Production Rate**: Items generated per minute (1-60)
- **Global Failure Rate**: Overall system failure probability (0-50%)

**Station-Specific Settings:**
- **Processing Time**: Min/Max processing duration (ms)
- **Failure Rate**: Station-specific failure probability
- **Retry Behavior**: Rework vs rejection policy

**System Settings:**
- **RNG Seed**: Fixed seed for reproducible simulations
- **Log Level**: Minimum severity for log display
- **Auto-save Interval**: Automatic state saving frequency
- **UI Update Rate**: Refresh frequency for real-time displays

#### Configuration Files

**Main Config**: `~/.prodline/config.json`
```json
{
  "version": 1,
  "production": {
    "mode": "threads",
    "bufferCapacity": 20,
    "rngSeed": 12345
  },
  "stations": [
    {
      "name": "Intake",
      "minProcessingTime": 100,
      "maxProcessingTime": 200,
      "failureRate": 0.01
    }
  ],
  "ui": {
    "theme": "light",
    "autoSave": true,
    "updateInterval": 1000
  }
}
```

**State File**: `~/.prodline/state.json`
```json
{
  "version": 1,
  "timestamp": "2024-11-17T10:30:00Z",
  "runtime": {
    "productsInProcess": [...],
    "bufferStates": {...},
    "stationStates": {...},
    "statistics": {...}
  }
}
```

### Advanced Features

#### IPC Mode
Enable inter-process communication mode:
1. Open Settings panel
2. Select "Processes+IPC" mode
3. Restart application
4. Monitor process list in Threads panel

In IPC mode, stations run as separate processes communicating via:
- **Named Pipes**: POSIX pipe-based product transfer
- **Unix Sockets**: Alternative IPC mechanism
- **Serialization**: JSON-based product data exchange

#### Custom Scenarios

**High-Load Testing:**
- Set production rate to maximum (60/min)
- Reduce buffer capacity (5-10 items)
- Increase failure rates (10-15%)
- Monitor for bottlenecks and blocking

**Quality Control Simulation:**
- Increase quality inspection failure rate
- Enable rework mode
- Monitor rework loop statistics
- Analyze throughput impact

**Performance Analysis:**
- Enable deterministic mode (fixed seed)
- Record baseline metrics
- Adjust various parameters
- Compare performance profiles

## API Documentation

### Core Classes

#### Product Class
```cpp
// Create new product
auto product = std::make_shared<Product>(ProductType::Washer);

// State management
product->advanceState();
product->setState(ProductState::AtAssembler);
product->addTraceEntry("Custom Processing Step");

// Serialization
QJsonObject json = product->toJson();
product->fromJson(json);
```

#### WorkStation Class
```cpp
class CustomStation : public WorkStation {
protected:
    bool processProduct(std::shared_ptr<Product> product) override {
        // Custom processing logic
        QThread::msleep(getRandomProcessingTime());
        
        if (shouldRejectProduct()) {
            return false; // Reject product
        }
        
        // Update product state
        product->advanceState();
        return true; // Accept product
    }
};
```

#### Buffer Class
```cpp
// Create thread-safe buffer
auto buffer = std::make_shared<Buffer<std::shared_ptr<Product>>>(20);

// Producer operations
buffer->push(product);        // Blocking
buffer->tryPush(product);     // Non-blocking

// Consumer operations
std::shared_ptr<Product> product;
buffer->pop(product);         // Blocking
buffer->tryPop(product);      // Non-blocking

// Status queries
int size = buffer->size();
bool full = buffer->isFull();
bool empty = buffer->isEmpty();
```

### Event System

#### Production Events
```cpp
// Connect to production controller events
connect(controller, &ProductionController::productFinished,
        this, &MyClass::onProductFinished);

connect(controller, &ProductionController::statisticsUpdated,
        this, &MyClass::onStatisticsUpdated);
```

#### Station Events
```cpp
// Connect to station events
connect(station, &WorkStation::stateChanged,
        this, &MyClass::onStationStateChanged);

connect(station, &WorkStation::productProcessed,
        this, &MyClass::onProductProcessed);
```

#### Logging Events
```cpp
// Connect to logging system
connect(logger, &Logger::logEntryAdded,
        this, &MyClass::onLogEntryAdded);

// Log custom messages
logger->info("Custom information message");
logger->warning("Warning message", "CustomCategory");
logger->error("Error occurred in processing");
```

## Testing

### Unit Tests
Located in `tests/` directory using Qt Test framework:

```bash
# Build and run tests
cd build
cmake --build . --target tests
./tests/ProductionLineTests
```

**Test Coverage:**
- Buffer thread-safety and blocking behavior
- Product state transitions and serialization
- WorkStation lifecycle management
- Persistence round-trip integrity
- Statistics calculation accuracy

### Integration Tests

**Scenario 1: Happy Path**
```bash
# Start production with default settings
# Verify products flow through all stations
# Check final product count matches expected
```

**Scenario 2: Quality Control**
```bash
# Set quality failure rate to 15%
# Monitor rework queue behavior
# Verify back-pressure handling
```

**Scenario 3: Performance Test**
```bash
# Process 100 products with timing constraints
# Verify UI remains responsive
# Check memory usage stays bounded
```

### Demo Scenarios

#### Basic Production Flow
1. Launch application
2. Configure 5 stations with default settings
3. Start production and observe flow
4. Products should progress: Intake → Assembler → Quality → Packaging → Shipping

#### Back-pressure Demonstration
1. Set small buffer sizes (5-10)
2. Slow down final station processing
3. Observe upstream stations blocking
4. Demonstrate queue management

#### Error Recovery
1. Configure 10% failure rate at Quality station
2. Enable rework mode
3. Monitor rework loop statistics
4. Verify system continues operating

#### IPC Mode
1. Enable Process+IPC mode in settings
2. Restart application
3. Monitor process list showing separate station processes
4. Verify products transfer between processes

## Known Issues

### Current Limitations
- IPC mode requires additional system configuration
- Chart performance may degrade with very high production rates
- Some UI elements may not scale properly on high-DPI displays
- Background thread cleanup can take up to 5 seconds on shutdown

### Workarounds
- **IPC Setup**: Ensure sufficient system pipe/socket limits
- **Performance**: Reduce chart update frequency for high-load scenarios
- **Display**: Use standard DPI settings for optimal experience
- **Shutdown**: Allow sufficient time for graceful termination

### Planned Improvements
- Enhanced IPC error handling and reconnection
- Optimized chart rendering for high-frequency updates
- Improved high-DPI display support
- Faster shutdown procedures with timeout mechanisms

## Defense Presentation

### Presentation Outline

**Slide 1: Title & Team**
- Project title and course information
- Team member names and contributions
- Delivery date and platform specifications

**Slide 2: System Overview**
- Production line simulation concept
- 5-station workflow diagram  
- Key OS concepts demonstrated

**Slide 3: Architecture & Design**
- UML class diagram overview
- Threading model explanation
- Synchronization mechanisms used

**Slide 4: Core Features Demonstration**
- Live demo of production line
- Real-time monitoring capabilities
- Station control and configuration

**Slide 5: Threading & Synchronization**
- Thread-safe buffer implementation
- QMutex, QSemaphore, QWaitCondition usage
- Producer-consumer pattern demonstration

**Slide 6: Advanced Features**
- IPC mode with process-based stations
- Persistence and state management
- Maintenance threads (Clean, Log, Stats)

**Slide 7: User Interface**
- GUI components and layout
- Real-time charts and monitoring
- Settings and configuration options

**Slide 8: Testing & Validation**
- Unit test coverage
- Integration test scenarios
- Performance benchmarks

**Slide 9: Challenges & Solutions**
- Technical difficulties encountered
- Solutions implemented
- Lessons learned

**Slide 10: Conclusions & Future Work**
- Project achievements
- Potential enhancements
- Real-world applications

### Q&A Preparation Checklist

**Technical Questions:**
- [ ] Explain thread synchronization mechanisms used
- [ ] Describe buffer implementation and thread safety
- [ ] Detail IPC communication protocols
- [ ] Justify design decisions and trade-offs
- [ ] Explain error handling and recovery procedures

**Implementation Questions:**
- [ ] Code walkthrough of critical sections
- [ ] Demonstration of station state transitions
- [ ] Explanation of persistence format and recovery
- [ ] Performance optimization techniques used
- [ ] Memory management and resource cleanup

**OS Concepts Questions:**
- [ ] Producer-consumer problem solution
- [ ] Deadlock prevention strategies
- [ ] Process vs thread trade-offs
- [ ] Synchronization primitive selection rationale
- [ ] Scalability considerations

### Demo Script

**Setup (2 minutes):**
1. Launch application with clean state
2. Open all dock panels (Logs, Metrics, Threads, Settings)
3. Explain UI layout and components

**Basic Operation (3 minutes):**
1. Configure stations with visible settings
2. Start production and narrate product flow
3. Show real-time updates in all panels
4. Demonstrate pause/resume functionality

**Advanced Features (3 minutes):**
1. Enable IPC mode and restart
2. Show process list with separate station processes
3. Demonstrate persistence by saving/loading state
4. Show maintenance thread activity in logs

**Error Handling (2 minutes):**
1. Increase failure rates
2. Show error recovery and rework loops
3. Demonstrate back-pressure with small buffers
4. Reset system and show clean recovery

---

## License
This project is developed for academic purposes as part of IF-4001 Operating Systems course. All rights reserved to the course instructors and participating students.

## Authors
- **Group 21 Members**: [Names to be added]
- **Course**: IF-4001 Operating Systems
- **Institution**: [Institution name]
- **Semester**: II-2025
