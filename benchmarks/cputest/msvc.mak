PROJECT = cputest
# zlib: set ZLIB_DIR to a folder with zlib.h and zlib.lib (x64, /MT)
!IF "$(ZLIB_DIR)" == ""
!ERROR Set ZLIB_DIR to a zlib build with zlib.h and zlib.lib
!ENDIF
EXTRA_INC = -I$(ZLIB_DIR)
EXTRA_LIBS = $(ZLIB_DIR)\zlib.lib
!include ../../env/msvc.mak
