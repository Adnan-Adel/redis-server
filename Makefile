CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
LDFLAGS  =

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build
BIN_DIR   = bin

SRCS   := $(wildcard $(SRC_DIR)/*.cpp)
OBJS   := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
DEPS   := $(OBJS:.o=.d)
TARGET  = $(BIN_DIR)/redis-server

.PHONY: all clean rebuild run

all: $(TARGET)

-include $(DEPS)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "  CXX  $<"
	@$(CXX) $(CXXFLAGS) -I$(INC_DIR) -MMD -MP -c $< -o $@

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "  LD   $@"
	@$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean
	$(MAKE) all

run: all
	./$(TARGET)
