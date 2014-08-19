##----------------------------------------------------------------------------##
# Copyright 2010 Mentor Graphics Corporation                                   #
#    All Rights Reserved.                                                      #
##----------------------------------------------------------------------------##

# Determine if a default value is needed for 'CONFIG'.  This check needs
# to be done at the very top because 'CONFIG' is used in a few other Makefiles
# which are included below.
ifeq ($(CONFIG),)
ifeq ($(origin CONFIG),command line)
$(error specifying an empty config 'CONFIG=' is not valid)
else
export CONFIG=debug
endif
endif

include tools/scripts/make/Makefile.vars
include tools/scripts/make/Makefile.commands

# Perform a few sanity checks.
ifneq ($(MAKECMDGOALS),help)

ifneq ($(MAKECMDGOALS),distclean)

ifeq ($(PLATFORM),)
ifneq ($(CURRENT_PLATFORM),)
export PLATFORM := $(CURRENT_PLATFORM)
else
$(error a target platform must be specified: PLATFORM=<name>)
endif
endif

# Check if the specified platform is available
$(if $(filter $(PLATFORM),$(available-platforms)),,\
    $(error Platform '$(PLATFORM)' does not exist, please use 'make help' to see list of available platforms))

ifeq ($(TOOLSET),)
ifneq ($(CURRENT_TOOLSET),)
export TOOLSET := $(CURRENT_TOOLSET)
else
$(error a toolset must be specified: TOOLSET=<name>)
endif
endif

# Check if the specified toolset is available
$(if $(filter $(TOOLSET),$(available-toolsets)),,\
    $(error Toolset '$(TOOLSET)' does not exist, please use 'make help' to see list of available toolsets))

ifeq ($(SYSTEM_HOME),)
export SYSTEM_HOME=$(CURDIR)
endif

ifeq ($(MAKECMDGOALS),custombsp)
ifeq ($(PLATFORM_CLONE),)
$(error a custombsp target platform must be specified: PLATFORM_CLONE=<name>)
endif
endif


ifeq ($(MAKECMDGOALS),nucsys)
ifeq ($(NUCSYS_DIR),)
$(error a target directory must be specified: NUCSYS_DIR=<name>)
endif
ifeq ($(NUCSYS_NAME),)
$(error a target system project name must be specified : NUCSYS_NAME=<name>)
endif
endif

endif

ifneq ($(CONFIG),)
ifeq ($(origin CONFIG),command line)
$(warning 'CONFIG' has been deprecated, please use 'USER_CONFIG' instead)
endif
ifneq ($(USER_CONFIG),)
ifeq ($(origin CONFIG),command line)
$(error 'CONFIG' and 'USER_CONFIG' can not be used together)
endif
endif
endif

ifneq ($(words $(valid-configs)),$(words $(configs)))
$(error the specified configuration CONFIG='$(CONFIG)' is not valid)
endif

# Handle 'CONF_FILE', which is an undocumented and deprecated option.
ifneq ($(CONF_FILE),)
$(warning 'CONF_FILE' has been deprecated, please use 'USER_CONFIG' instead)
endif

endif

ifneq ($(CONF_FILE),)
existing-config-files += $(CONF_FILE)
endif

# Add in user configurations, if any.
ifneq ($(USER_CONFIG),)
existing-config-files += $(USER_CONFIG)
endif

# Apply the appropriate configurations, if specified.
ifeq ($(words $(existing-config-files)),1)
export CONF_FILE_COMMAND=-f $(existing-config-files)
else ifneq ($(words $(existing-config-files)),0)
export CONF_FILE_COMMAND=-f $(subst $(space),$(comma),$(existing-config-files))
endif

# Set the optional transforms needed by fuse for building Nucleus.
ifeq ($(MAKECMDGOALS),custombsp)
export OPT_TRANSFORMS=PlatformCloneTransform:PropertyLoaderTransform:PlatformLoadTransform
else
export OPT_TRANSFORMS=HTMLReportTransform:BuildPropertiesWriterTransform:NucleusToolsetDefsTransform:QtConfigTransform:NucleusMakefileTransform:RequirementCheckerTransform:NucleusConfigfileTransform:RegistryInitTransform:NucleusCustomTransform:DriverAutoEnableTransform:PropertyLoaderTransform:PlatformLoadTransform
endif

# Controls the verbosity of the build output.
ifeq ($(VERBOSE),1)
export Q = # nil
else
export Q = @
endif

# Disable builtin rules and variables.
MAKEFLAGS += -r -R --no-print-directory

# Work around crippled Windows 'make -j' behavior, since '-j n' is always 
# set to '-j 1' on Windows.
ifneq ($(ComSpec),)

ifneq ($(JOBS),)
make-jobs = -j $(JOBS)
else
make-jobs = $(if $(findstring j,$(MAKEFLAGS)),-j $(NUMBER_OF_PROCESSORS),)
endif

endif

.NOTPARALLEL :
.PHONY : all fuse init clean config distclean help custombsp

all : fuse init
	$(Q)$(MAKE) $(make-jobs) -f tools/scripts/make/Makefile.entry
fuse :
ifeq ($(TOOLSET),renesas_sh)
	$(Q)$(MAKE) -W .metadata -f tools/scripts/make/Makefile.fuse
else
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.fuse
endif
init :
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.init
clean: 
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.clean
distclean:
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.clean DIST=1
config:
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.fuse config
help:
	@$(call print,Available Make Targets:)
	@$(call print,     all            - Builds everything for the specified platform)
	@$(call print,                      and toolset. (default))
	@$(call print,     config         - Configures the current tree to the default values or)
	@$(call print,                      whatever is defined through 'USER_CONFIG'.)
	@$(call print,     clean          - Cleans the 'output' directory for the specified)
	@$(call print,                      platform/toolset.)
	@$(call print,     distclean      - Cleans the whole 'output' directory and any)
	@$(call print,                      generated files.)
	@$(call print,     custombsp      - Makes a simple clone named 'PLATFORM_CLONE' based on)
	@$(call print,                      the platform value 'PLATFORM'.)
	@$(call print,     nucsys         - Creates a nucleus system project based on )
	@$(call print,                      the platform value 'PLATFORM'.)
	@$(call print,     help           - Display the help for the make system.)
	@$(call print,) 
	@$(call print,Available Options:)
	@$(call print,     PLATFORM       - Specifies what hardware platform to build for.)
	@$(call print,     TOOLSET        - Specifies what toolset to build with.)
	@$(call print,     USER_CONFIG    - Specifies the name of a file holding a user)
	@$(call print,                      defined configuration.)
	@$(call print,     VERBOSE        - Specifies whether verbose output should be used)
	@$(call print,                      (default: 0))
	@$(call print,     PLATFORM_CLONE - Specifies the name for the platform cloned from)
	@$(call print,                      'PLATFORM'.)
	@$(call print,     JOBS           - The number of parallel jobs to allow at once.)
	@$(call print,                      NOTE: This parameter is only honored on Windows.)
	@$(call print,     NUCSYS_DIR     - Target directory to create Nucleus System project in.)
	@$(call print,     NUCSYS_NAME    - Name of Nucleus System Project.)
	@$(call print,) 
	@$(call print,Available Platforms:)
	@$(foreach plat, $(available-platforms),\
               $(call print,     $(plat))$(SEP))
	@$(call print,) 
	@$(call print,Available Toolsets:)
	@$(foreach tool, $(available-toolsets),\
               $(call print,     $(tool))$(SEP))
	@$(call print,) 
custombsp: 
	@$(call print,Cloning target)
	$(Q)$(MAKE) -f tools/scripts/make/Makefile.clone config
nucsys: distclean
	@$(call print,Creating Nucleus System Project: $(NUCSYS_NAME))
	@$(call print,at: $(NUCSYS_DIR))
	@$(call print,for platform: $(PLATFORM))
	@$(call print,Copying files ....)
ifneq ($(findstring :,$(NUCSYS_DIR)),)
ifneq ($(findstring $(realpath .),$(subst \,/,$(NUCSYS_DIR))),)
	$(error Please specify the path of NUCSYS_DIR outside of installed source directory)
else
	@$(call MKDIR,$(NUCSYS_DIR))
endif
else 
ifneq ($(findstring $(realpath .),$(realpath $(subst \,/,$(NUCSYS_DIR)))),)
	$(error Please specify the path of NUCSYS_DIR outside of installed source directory)
else
	@$(call MKDIR,$(NUCSYS_DIR))
endif
endif
	@$(call create_dir,$(NUCSYS_DIR)/$(NUCSYS_NAME))
	@$(call create_dir,$(NUCSYS_DIR)/$(NUCSYS_NAME)/nucleus)
	@$(call copy_dir_ignore,.,$(NUCSYS_DIR)/$(NUCSYS_NAME)/nucleus,bsp)
	@$(call MKDIR,$(NUCSYS_DIR)/$(NUCSYS_NAME)/nucleus/bsp/$(PLATFORM))
	@$(call CP, ./bsp/.metadata,$(NUCSYS_DIR)/$(NUCSYS_NAME)/nucleus/bsp/.metadata)
	@$(call CPDIR,./bsp/$(PLATFORM)/*,$(NUCSYS_DIR)/$(NUCSYS_NAME)/nucleus/bsp/$(PLATFORM))
	@$(call print,Done.)

