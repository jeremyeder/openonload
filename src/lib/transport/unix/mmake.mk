ifeq ($(ISA),amd64)
BUILD_TREE_COPY	:= mapfile.lp64
endif

ifeq ($(ISA),i386)
BUILD_TREE_COPY	:= mapfile.ilp32
endif

TARGET		:= libcitransport0.so
MMAKE_TYPE	:= DLL

LDEP	:= $(CITPCOMMON_LIB_DEPEND) $(CIIP_LIB_DEPEND) \
	$(CIUL_LIB_DEPEND) $(CITOOLS_LIB_DEPEND)

LLNK	:= $(LINK_CITPCOMMON_LIB) $(LINK_CIIP_LIB) \
	$(LINK_CIUL_LIB) $(LINK_CITOOLS_LIB)


LIB_SRCS	:=			\
		startup.c		\
		log_fn.c		\
		sys.c			\
		sockcall_intercept.c	\
		onload_ext_intercept.c	\
		zc_intercept.c          \
		stackname.c		\
		fdtable.c		\
		protocol_manager.c	\
		closed_fd.c		\
		tcp_fd.c		\
		udp_fd.c		\
		pipe_fd.c		\
		epoll_common.c		\
		epoll_fd.c		\
		epoll_fd_b.c		\
		netif_init.c		\
		exec.c			\
		environ.c		\
		common_fcntl.c		\
		trampoline.c		\
		wqlock.c		\
		poll_select.c		\
		debug.c


MMAKE_OBJ_PREFIX := ci_tp_unix_
LIB_OBJS	:= $(LIB_SRCS:%.c=$(MMAKE_OBJ_PREFIX)%.o)

ifneq ($(NO_TRAMPOLINE),1)
LIB_OBJS	+= $(MMAKE_OBJ_PREFIX)trampoline_asm.o
else
# it is really necessary for trampoline.c only
cppflags	+= -DNO_TRAMPOLINE=1
endif

MMAKE_CFLAGS 	+= -DONLOAD_EXT_VERSION_MAJOR=$(ONLOAD_EXT_VERSION_MAJOR)
MMAKE_CFLAGS 	+= -DONLOAD_EXT_VERSION_MINOR=$(ONLOAD_EXT_VERSION_MINOR)
MMAKE_CFLAGS 	+= -DONLOAD_EXT_VERSION_MICRO=$(ONLOAD_EXT_VERSION_MICRO)

all: $(TARGET)

lib: $(TARGET)

clean:
	@$(MakeClean)

# This tells the linker which symbols to include in the dynamic symbol
# table.  It is useful to omit this to see whether the "hidden" attribute
# is being used appropriately.  Just do "make EXPMAP="
EXPMAP := -Wl,--version-script=$(TOP)/src/lib/transport/unix/exports.map

$(TARGET): $(LIB_OBJS) $(LDEP) exports.map
	(libs="$(LLNK) -e onload_version_msg $(EXPMAP)"; \
	$(MMakeLinkPreloadLib))
