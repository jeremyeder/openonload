
DRIVER_SUBDIRS	     := lib driver

ifeq ($(WINDOWS),1)
OTHER_SUBDIRS        := app
SUBDIRS              := include lib tools tests
DRIVER_SUBDIRS       += tools
INSTALLER_SUBDIRS    := tools
endif

ifeq ($(GNU),1)
SUBDIRS              := include lib app driver tools tests
endif

ifeq ($(SOLARIS),1)
SUBDIRS              := lib app tools tests
endif

ifeq ($(FREEBSD), 1)
SUBDIRS              := include lib app driver tools tests
endif

ifeq ($(MACOSX), 1)
SUBDIRS              := lib tools tests
endif

ifeq ($(LINUX),1)
DRIVER_SUBDIRS	     := lib driver
OTHER_DRIVER_SUBDIRS := tests
endif

ifeq ($(GLD),1)
SUBDIRS              := driver
endif

ifeq ($(DOS),1)
SUBDIRS              := driver
endif

ifeq ($(DOS32_UTILS)$(LINUX_UIO_UTILS),1)
SUBDIRS              := tools
endif

ifeq ($(SPECIAL_TOP_RULES),1)

all:    special_top_all

clean:    special_top_clean

else

all:
ifeq ($(LINUX),1)
ifneq ($(GNU),1)
	# Build both autocompat.h files: linux_net and linux_affinity.
	$(MAKE) -C driver/linux_net
	$(MAKE) -C driver/linux_affinity \
		$(BUILDPATH)/driver/linux_affinity/autocompat.h
endif
endif
	+@$(MakeSubdirs)

clean:
	@$(MakeClean)
endif
