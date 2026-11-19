#######################
#### Start of File ####
#######################
# --------------------------------------------------------------- 
# Makefile Contents: dr_utils nmake settings for Windows
# C/C++ Compiler Used: Visual Studio 2022 and 2026
# Produced By: DataReel Software Development Team
# File Creation Date: 11/18/2026
# Date Last Modified: 11/18/2026
# --------------------------------------------------------------- 
# Each tool's msvc.mak sets PROJECT and includes this file. A tool
# that needs another library sets EXTRA_INC and EXTRA_LIBS first.
#
# DATAREEL_DIR: a DataReel 6.4.1 tree with the library built in
# winslib (gxcode64.lib for 64BITCFG=1 FINAL=1). Use the same
# settings (64BITCFG, FINAL, USESSL) as the library build.
!IF "$(DATAREEL_DIR)" == ""
!ERROR Set DATAREEL_DIR to a DataReel 6.4.1 tree with the winslib library built
!ENDIF

!IF "$(FINAL)" == ""
FINAL = 1
!ENDIF
!IF "$(64BITCFG)" == ""
64BITCFG = 1
!ENDIF

GCODE_LIB_DIR = $(DATAREEL_DIR)
GCODE_LIB_PATH = $(DATAREEL_DIR)

!IF "$(USESSL)" == "1"
!include $(DATAREEL_DIR)/env/openssl_windows.env
!ENDIF
!include $(DATAREEL_DIR)/winslib/msvc.env

COMMON_DIR = ..\..\common
COMPILE_FLAGS = $(COMPILE_FLAGS) -I$(COMMON_DIR) $(EXTRA_INC)
DR_CXX = cl

all:	$(PROJECT)$(EXE_EXT)

!include project.mak

GXCODE_LIB_PATH = $(DATAREEL_DIR)$(PATHSEP)winslib
!IF "$(64BITCFG)" == "1" && "$(FINAL)" == "1"
LIB_GXCODE = $(GXCODE_LIB_PATH)$(PATHSEP)gxcode64.lib
!ENDIF
!IF "$(64BITCFG)" == "1" && "$(FINAL)" == "0"
LIB_GXCODE = $(GXCODE_LIB_PATH)$(PATHSEP)gxcode64d.lib
!ENDIF
!IF "$(64BITCFG)" == "0" && "$(FINAL)" == "1"
LIB_GXCODE = $(GXCODE_LIB_PATH)$(PATHSEP)gxcode32.lib
!ENDIF
!IF "$(64BITCFG)" == "0" && "$(FINAL)" == "0"
LIB_GXCODE = $(GXCODE_LIB_PATH)$(PATHSEP)gxcode32d.lib
!ENDIF

# Shared network helpers
drnet$(OBJ_EXT):	$(COMMON_DIR)\drnet.cpp $(COMMON_DIR)\drnet.h
	$(DR_CXX) $(COMPILE_ONLY) $(COMPILE_FLAGS) $(COMMON_DIR)\drnet.cpp

# Shared benchmark sockets
drsock$(OBJ_EXT):	$(COMMON_DIR)\drsock.cpp $(COMMON_DIR)\drsock.h
	$(DR_CXX) $(COMPILE_ONLY) $(COMPILE_FLAGS) $(COMMON_DIR)\drsock.cpp

# setargv.obj expands wildcards (file_purge *.log)
$(PROJECT)$(EXE_EXT):	$(OBJECTS)
	$(LINKER) $(LINKER_FLAGS) $(OBJECTS) $(LIB_GXCODE) \
	$(EXTRA_LIBS) $(LINK_LIBRARIES) ws2_32.lib iphlpapi.lib setargv.obj /OUT:$@

clean:
	if exist *.obj del *.obj
	if exist *.pdb del *.pdb
	if exist $(PROJECT)$(EXE_EXT) del $(PROJECT)$(EXE_EXT)
# --------------------------------------------------------------- 
#####################
#### End of File ####
#####################
