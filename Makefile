# 编译器与编译选项
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# 源文件与目标文件
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
TARGET = toyc_lexer

# 默认目标
all: $(TARGET)

# 链接目标
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 清理编译产物
clean:
	rm -f $(TARGET)

# 重新构建
rebuild: clean all

.PHONY: all clean rebuild