# Objects for file_hours (shared by makefile and msvc.mak)
$(PROJECT)$(OBJ_EXT):	$(PROJECT).cpp
	$(DR_CXX) $(COMPILE_ONLY) $(COMPILE_FLAGS) $(PROJECT).cpp

OBJECTS = $(PROJECT)$(OBJ_EXT)
