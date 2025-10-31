# 编译器与编译选项
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# 源文件与目标
SRC = src/lexer.cpp
TARGET = toyc_lexer

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

# 清理
clean:
	rm -f $(TARGET)

# 重新构建
rebuild: clean all

.PHONY: all clean rebuild