
TARGET		:= $(CITOOLS_LIB)
MMAKE_TYPE	:= LIB

LIB_SRCS	:= \
		buddy.c \
		buffer.c \
		bufrange.c \
		crc16.c \
		crc32.c \
		crc32c.c \
		toeplitz.c \
		cpu_features.c \
		dllist.c \
		eth_addr.c \
		fail.c \
		fifo_grow_lock.c \
		hex_dump.c \
		hex_dump_to_raw.c \
		ipcsum.c \
		ip_csum_partial.c \
		memchk.c \
		tcp_checksum.c \
		udp_checksum.c \
		icmp_checksum.c \
		log.c \
		log_buffer.c \
		log_nth.c \
		log_unique.c \
		memcpy.c \
		parse_eth_addr.c \
		sys_fail.c \
		pktdump.c \
		ip_addr.c \
		csum_copy2.c \
		csum_copy_iovec.c \
		csum_copy_to_iovec.c \
		copy_iovec.c \
		copy_to_iovec.c \
		ip_csum_precompute.c \
		tcp_csum_precompute.c \
		udp_csum_precompute.c \
		ippacket.c \
		dump_select.c

ifeq ($(DRIVER),1)
LIB_SRCS	+= drv_log_fn.c drv_get_cpu_khz.c memleak_debug.c
LIB_SRCS	+= drv_thread.c
else
LIB_SRCS	+= cithread.c get_cpu_khz.c log_fn.c log_file.c
LIB_SRCS	+= dump_select.c
ifneq ($(ONLOAD_ONLY),1)
LIB_SRCS	+= rbmw_parse.c \
		rbmw_simulator.c \
		rbmw_dumper.c
endif

ifneq ($(WINDOWS),1)
LIB_SRCS	+= glibc_version.c
else
LIB_SRCS	+= mswin_helper.c

endif

endif


ifndef MMAKE_NO_RULES

MMAKE_OBJ_PREFIX := ci_tools_
LIB_OBJS	 := $(LIB_SRCS:%.c=$(MMAKE_OBJ_PREFIX)%.o)

ifneq ($(WINDOWS),1)
LIB_OBJS	+=  ci_tools_asm_memcpy.o
LIB_OBJS	+=  ci_tools_cas64u.o
endif

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
	$(MAKE) $(MMAKE_KBUILD_ARGS) SUBDIRS=$(BUILDPATH)/lib/citools _module_$(BUILDPATH)/lib/citools

clean:
	@$(MakeClean)
	rm -f lib.a
endif

ifdef MMAKE_IN_KBUILD
LIB_OBJS := $(LIB_SRCS:%.c=%.o)
lib-y    := $(LIB_OBJS)
endif
