########################################
# make 設定
.PHONY: all relink cleant clean debug run
COMMAND_DEL = rm

########################################
# Debug or Release
#BUILD_MODE = Debug
BUILD_MODE = Release

ifeq ($(BUILD_MODE),Debug)
  DEBUG_FLAGS = -g
endif

########################################
# コンパイラ設定
CC = g++
CFLAGS = -O2 -mwindows -w $(DEBUG_FLAGS)
DXSDK_PATH = C:\Program Files (x86)\Microsoft DirectX 9.0 SDK (Summer 2004)
DXSDK_INCLUDE_PATH = $(DXSDK_PATH)/include
INCLUDES = -I"$(DXSDK_INCLUDE_PATH)"

########################################
# リンカ設定
CLIBS = -lgdi32 -lole32 -lcomdlg32 -limm32 -lwinmm -lvfw32
D3D8LIBS = -ldxguid -ld3d8 -ld3dx8d -ld3dxof -ldinput8 -ldinput -ldsound

########################################
# ビルド入出力設定
OBJ_DIR = $(BUILD_MODE)_gcc
TARGET_DIR = RailSim2
TARGET = $(TARGET_DIR)/RailSim2_$(BUILD_MODE)_gcc.exe

RS2_SRCFILES = $(wildcard *.cpp)
RS2_OBJFILES = $(addprefix $(OBJ_DIR)/, $(RS2_SRCFILES:.cpp=.o))
RS2_DEPFILES = $(addprefix $(OBJ_DIR)/, $(RS2_SRCFILES:.cpp=.d))

UDX_SRCFILES = $(wildcard lib/*.cpp)
UDX_OBJFILES = $(addprefix $(OBJ_DIR)/, $(UDX_SRCFILES:.cpp=.o))
UDX_DEPFILES = $(addprefix $(OBJ_DIR)/, $(UDX_SRCFILES:.cpp=.d))

RES_DIR = res
RES_FILES = $(wildcard $(RES_DIR)/*.*)
RES_SRCFILE = RailSim2.rc
RES_OBJFILE = $(OBJ_DIR)/RailSim2.res

########################################
# ビルド

all: $(TARGET)

$(TARGET): $(RS2_OBJFILES) $(UDX_OBJFILES) $(RES_OBJFILE)
	$(CC) $(CFLAGS) -o $@ $(RS2_OBJFILES) $(UDX_OBJFILES) \
		$(D3D8LIBS) $(CLIBS) $(RES_OBJFILE)

$(OBJ_DIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) -c $<

$(RES_OBJFILE): $(RES_SRCFILE) $(RES_FILES)
	windres -O coff $(RES_SRCFILE) $(RES_OBJFILE)

relink: cleant $(TARGET)

########################################
# 生成したファイルの削除

cleant:
	-del $(subst /,\,$(TARGET))

clean: cleant
	-del $(OBJ_DIR)\*.res
	-del $(OBJ_DIR)\*.o
	-del $(OBJ_DIR)\*.d
	-del $(OBJ_DIR)\lib\*.o
	-del $(OBJ_DIR)\lib\*.d

########################################
# デバッグ・実行コマンド

debug: $(TARGET)
	gdb ./$(TARGET)

run: $(TARGET)
	./$(TARGET)

########################################
# 依存関係の自動チェック

%.h:
	@echo DELETED HEADER FILE: $@

$(OBJ_DIR)/%.d: %.cpp
	-set /P X=$@ $(OBJ_DIR)/<NUL>$@
	$(CC) $(CFLAGS) -MM $(INCLUDES) $< >> $@ || del /F $(subst /,\,$@)

$(OBJ_DIR)/lib/%.d: %.cpp
	-set /P X=$@ $(OBJ_DIR)/lib/<NUL>$@
	$(CC) $(CFLAGS) -MM $(INCLUDES) $< >> $@ || del /F $(subst /,\,$@)

include $(RS2_DEPFILES) $(UDX_DEPFILES)
