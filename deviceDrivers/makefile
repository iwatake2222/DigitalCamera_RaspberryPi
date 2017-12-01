include ../makefile_config

TARGET = deviceDrivers.a
SRCS = ddIli9341Spi.cpp ddTpTsc2046Spi.cpp ddCamera.cpp
INCLUDES = -I../
SUBS =
SUBDIRS =

all: sub_target $(TARGET)

$(TARGET): $(OBJS) $(SUBS)
	$(AR) r $@ $(OBJS) $(SUBS)

.PHONY: sub_target
sub_target:
ifdef SUBDIRS
	$(foreach subdir,$(SUBDIRS),cd $(subdir) && $(MAKE) && cd ../ &&) cd ./
endif

.PHONY: clean
clean:
	$(DEL) *.o *.a *.exe *.dll *.so
ifdef SUBDIRS
	$(foreach subdir,$(SUBDIRS),cd $(subdir) && $(MAKE) clean && cd ../ &&) cd ./
endif

