
TARGET		:= $(EFHW_LIB)
MMAKE_TYPE	:= LIB

LIB_SRCS	:= nic.c \
		   eeprom.c \
		   eventq.c \
		   falcon.c \
		   falcon_mac.c \
		   falcon_iscsi.c \
		   falcon_spi.c \
		   ef10.c

ifndef MMAKE_NO_RULES

MMAKE_OBJ_PREFIX := ef_hw_
LIB_OBJS	 := $(LIB_SRCS:%.c=$(MMAKE_OBJ_PREFIX)%.o)

all: $(TARGET)

lib: $(TARGET)

clean:
	@$(MakeClean)

$(TARGET): $(LIB_OBJS)
	$(MMakeLinkStaticLib)
endif


######################################################
# linux kbuild support
#
ifdef MMAKE_USE_KBUILD
all:
	 $(MAKE) $(MMAKE_KBUILD_ARGS) SUBDIRS=$(BUILDPATH)/lib/efhw _module_$(BUILDPATH)/lib/efhw
clean:
	@$(MakeClean)
	rm -f lib.a
endif

ifdef MMAKE_IN_KBUILD
LIB_OBJS := $(LIB_SRCS:%.c=%.o)
lib-y    := $(LIB_OBJS)
endif
