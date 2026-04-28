LUA_ROOT      ?= /ucrt64
CXX           ?= g++
MSYS_INCLUDES ?= -I/ucrt64/include/winrt

ifeq ($(wildcard $(LUA_ROOT)/lib/liblua.a),$(LUA_ROOT)/lib/liblua.a)
    LUA_LIB = -llua
else ifeq ($(wildcard $(LUA_ROOT)/lib/liblua54.a),$(LUA_ROOT)/lib/liblua54.a)
    LUA_LIB = -llua54
else ifeq ($(wildcard $(LUA_ROOT)/lib/liblua5.4.a),$(LUA_ROOT)/lib/liblua5.4.a)
    LUA_LIB = -llua5.4
else
    $(error Could not find Lua library in $(LUA_ROOT)/lib)
endif

CXXFLAGS = -O2 -std=c++20 -Wall -Wextra -fms-extensions -fpermissive \
			-D_AMD64_ -DWIN64 -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
			-Iinclude -I"$(LUA_ROOT)/include" \
			$(MSYS_INCLUDES)

LDFLAGS = -shared -static -static-libgcc -static-libstdc++ \
	-L"$(LUA_ROOT)/lib" $(LUA_LIB) -lwindowsapp -lshell32 \
	-lole32 -luuid -lshlwapi -lpropsys -lruntimeobject

SRC = $(wildcard src/api/*.cpp src/api/lua/*.cpp src/app/*.cpp src/utils/*.cpp \
               src/runtime/*.cpp src/runtime/notifications/*.cpp \
               src/runtime/handlers/*.cpp src/*.cpp)

OBJDIR = build
OBJ = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))

TARGET = lunalify_core.dll
BIN = bin


all: $(BIN) $(TARGET)
	@mv -f $(TARGET) $(BIN)/$(TARGET)

$(BIN):
	@mkdir -p $(BIN)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJDIR) $(BIN) dist *.zip

release: all
	@powershell -ExecutionPolicy Bypass -File scripts/release.ps1
