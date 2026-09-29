CXX = clang++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread
ISYSROOT = $(shell xcrun --show-sdk-path 2>/dev/null || echo "")
ifneq ($(wildcard /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk),)
    ISYSROOT = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
endif
ifneq ($(ISYSROOT),)
    SYSROOT_FLAG = -isysroot $(ISYSROOT)
else
    SYSROOT_FLAG =
endif

INCLUDES = -Ibackend/include

SRCS = backend/src/knapsack_solver.cpp \
       backend/src/greedy_solver.cpp \
       backend/src/rank_tree.cpp \
       backend/src/min_heap.cpp \
       backend/src/dsu.cpp

SERVER_SRC = backend/src/server.cpp
TARGET = dreamxi_server

all: $(TARGET)

$(TARGET): $(SRCS) $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SYSROOT_FLAG) $(INCLUDES) $(SRCS) $(SERVER_SRC) -o $(TARGET)

test_solvers: $(SRCS) test_solvers.cpp
	$(CXX) $(CXXFLAGS) $(SYSROOT_FLAG) $(INCLUDES) backend/src/knapsack_solver.cpp backend/src/greedy_solver.cpp test_solvers.cpp -o test_solvers

test_ds: $(SRCS) test_data_structures.cpp
	$(CXX) $(CXXFLAGS) $(SYSROOT_FLAG) $(INCLUDES) backend/src/knapsack_solver.cpp backend/src/rank_tree.cpp backend/src/min_heap.cpp backend/src/dsu.cpp test_data_structures.cpp -o test_ds

clean:
	rm -f $(TARGET) test_solvers test_ds

run: $(TARGET)
	./$(TARGET) 8080

.PHONY: all clean run test_solvers test_ds
