#######################
#### Start of File ####
#######################
# --------------------------------------------------------------- 
# Makefile Contents: dr_utils make settings for Linux (GNU make)
# C/C++ Compiler Used: GNU
# Produced By: DataReel Software Development Team
# File Creation Date: 11/18/2026
# Date Last Modified: 11/18/2026
# --------------------------------------------------------------- 
# Each tool's makefile sets PROJECT and includes this file.
#
# DATAREEL_DIR: a DataReel 6.4.1 tree with the library built
# (cd $DATAREEL_DIR/build && make && make install). Set it in the environment or on
# the make command line. Use the same settings (USE_OPEN_SSL, FINAL,
# LARGEFILES) as the library build.
ifndef DATAREEL_DIR
$(error Set DATAREEL_DIR to a DataReel 6.4.1 tree with lib/libdatareel.a built)
endif

GCODE_LIB_DIR = $(DATAREEL_DIR)
include $(DATAREEL_DIR)/env/linux.env

COMMON_DIR = ../../common
COMPILE_FLAGS += -I$(COMMON_DIR)
DR_CXX = $(CXX)
LIB_GXCODE = $(DATAREEL_DIR)/lib/libdatareel.a

all:	$(PROJECT)

include project.mak

# Shared network helpers
drnet$(OBJ_EXT):	$(COMMON_DIR)/drnet.cpp $(COMMON_DIR)/drnet.h
	$(DR_CXX) $(COMPILE_ONLY) $(COMPILE_FLAGS) $(COMMON_DIR)/drnet.cpp

$(PROJECT):	$(OBJECTS) $(LIB_GXCODE)
	$(DR_CXX) $(COMPILE_FLAGS) $(OBJECTS) $(LIB_GXCODE) \
	$(LINK_LIBRARIES) $(OUTPUT) $(PROJECT) $(LINKER_FLAGS)

clean:
	rm -f *.o $(PROJECT)
# --------------------------------------------------------------- 
#####################
#### End of File ####
#####################
