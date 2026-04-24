# Path to Lua installation (headers + libs)
LUA_ROOT = V:/lua-5.4.8/lua-5.4.8
# Path to MinGW toolchain binaries (compiler, linker, etc.)
MINGW_BIN = V:/winlibs-x86_64-posix-seh-gcc-15.1.0-mingw-w64ucrt-13.0.0-r4/mingw64/bin
# C++ compiler (g++ from MSYS2 UCRT64 environment)
CXX = V:/msys64/ucrt64/bin/g++


# Detect correct Lua library name automatically
ifeq ($(wildcard $(LUA_ROOT)/lib/liblua.a),$(LUA_ROOT)/lib/liblua.a)
    LUA_LIB = -llua
else ifeq ($(wildcard $(LUA_ROOT)/lib/liblua54.a),$(LUA_ROOT)/lib/liblua54.a)
    LUA_LIB = -llua54
else ifeq ($(wildcard $(LUA_ROOT)/lib/liblua5.4.a),$(LUA_ROOT)/lib/liblua5.4.a)
    LUA_LIB = -llua5.4
else
    $(error Could not find Lua library in $(LUA_ROOT)/lib)
endif


# Compiler flags
CXXFLAGS = -O2 -std=c++20 -Wall -Wextra -fms-extensions -fpermissive \
			-D_AMD64_ -DWIN64 -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
			-Iinclude -I"$(LUA_ROOT)/include" \
			-I"V:/msys64/ucrt64/include" \
			-I"V:/msys64/ucrt64/include/winrt"

# Linker flags
LDFLAGS = -shared -static -static-libgcc -static-libstdc++ \
	-L"$(LUA_ROOT)/lib" $(LUA_LIB)  -lwindowsapp -lshell32 \
	-lole32 -luuid -lshlwapi -lpropsys -lruntimeobject
# Source files
SRC = src/main.cpp src/core/common.cpp src/core/identity.cpp \
	src/toast/toast.cpp src/toast/base_toast.cpp src/toast/ToastEventManager.cpp \
	src/core/daemon/daemon_manager.cpp

# Object files
OBJ = $(SRC:.cpp=.o)
# Output target
TARGET = lunalify_core.dll
BIN = bin
RELEASE_NAME = lunalify-v0.1.0-win64


all: $(BIN) $(TARGET)
	@move /y $(TARGET) $(BIN) >nul

release: all
	@echo Creating release package structure...
	@if exist "dist" rmdir /s /q "dist"
	@if exist "$(RELEASE_NAME).zip" del "$(RELEASE_NAME).zip"

	@mkdir "dist\lunalify"
	@mkdir "dist\lunalify\bin"
	@copy /y "$(BIN)\$(TARGET)" "dist\lunalify\bin\"
	@xcopy /s /e /y "lua\lunalify\*" "dist\lunalify\"

	@xcopy /s /e /y "docs\*" "dist\lunalify\docs\"
	@copy /y "README.md" "dist\lunalify\"
	@copy /y "LICENSE" "dist\lunalify\"
	@echo Zipping package...
	@powershell -ExecutionPolicy Bypass -Command "Compress-Archive -Path 'dist\lunalify' -DestinationPath './$(RELEASE_NAME).zip' -Force"

	@echo Release package created: $(RELEASE_NAME).zip
	@if exist "dist" rmdir /s /q "dist"

$(BIN):
	@if not exist $(BIN) mkdir $(BIN)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@if exist bin del /f /Q bin\*
	@del /f src\*.o src\core\*.o src\toast\*.o src\core\daemon\*.o 2>nul
	@if exist dist rmdir /s /q dist
	@if exist $(RELEASE_NAME).zip del $(RELEASE_NAME).zip
