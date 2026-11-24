# ------------------------------------------------------------------------
# Basics
# ------------------------------------------------------------------------

# CUDA architecture
CUDA_ARCH			:= -gpu=cc80

# Directories
SRC_DIR				:= src
INC_DIR				:= inc
BUILD_DIR			:= build
BIN_DIR				:= bin

# Files
CPP_SRCS     		:= $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJS     		:= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS))
EXES        		:= $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.x,$(CPP_SRCS))


# ------------------------------------------------------------------------
# Compiler Flags
# ------------------------------------------------------------------------

MPICXX_FLAGS		:= -std=c++20 -acc $(CUDA_ARCH) -Minfo=accel

RELFLAGS			:= -O3
DEVFLAGS			:= -O0 -g -gpu=debug -Wall -Wextra -pedantic

MPICXX_FLAGS		:= $(MPICXX_FLAGS) $(RELFLAGS)

INCLUDE_FLAGS		:= -I$(INC_DIR)
LIBS				:= -lcublas -lcurand -lcudart -lnvToolsExt -L${CUDA_HOME}/lib64


# ------------------------------------------------------------------------
# Build rules
# ------------------------------------------------------------------------
all: $(EXES)

$(BIN_DIR)/%.x: $(BUILD_DIR)/%.o | $(BIN_DIR)
	$(MPICXX) $(MPICXX_FLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(MPICXX) $(MPICXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean