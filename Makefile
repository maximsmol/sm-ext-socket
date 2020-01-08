ifeq ($(OS),Windows_NT)
	$(error No windows support with this makefile)
endif

SRCPATH = src
DISTPATH = dist
WORLDPATH = world
BLD_PATH = bld

# = = =
# Configure
#
TARGET = smsocket
#
LIBNAME = socket
LIBPREFIX =
LIBSUFFIX = .ext
#
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBEXT = so
endif
ifeq ($(UNAME_S),Darwin)
	LIBEXT = dylib
endif
#
# = = =

# DON'T CHANGE
LIBFILENAME = $(LIBPREFIX)$(LIBNAME)$(LIBSUFFIX).$(LIBEXT)

# = = =
# Configure
#
LIBPATH = $(DISTPATH)/$(LIBFILENAME)
INCPATH = $(DISTPATH)/socket.inc

SMPATH = $(WORLDPATH)/sourcemod
BOOSTPATH = $(WORLDPATH)/libboost_thread.a
#
COMPILER = clang++
CPP_FLAGS = -m32
#
CPP_DEBUG_FLAGS = -O0 -g -glldb
CPP_RELEASE_FLAGS = -Ofast
#
#
LINKER = $(COMPILER)
LINK_FLAGS = -m32 -arch i386 -dynamiclib -install_name @rpath/${LIBFILENAME}
#
# = = =

# DON'T CHANGE

TARGET_SRC_PATH = $(SRCPATH)/$(TARGET)
TARGET_BLD_PATH = $(BLD_PATH)/$(TARGET)

CPP_SRCS =
CPP_SRCS += Socket
CPP_SRCS += Extension
CPP_SRCS += sdk/smsdk_ext
CPP_SRCS += SocketHandler
CPP_SRCS += Callback
CPP_SRCS += CallbackHandler
CPP_SRCS_CPP = $(TARGET_SRC_PATH)/$(CPP_SRCS:%=%.cpp)

SM_INCLUDES = . amtl amtl/amtl
SOURCEMOD_INCLUDES = $(SMPATH)/sourcepawn/include/. $(SM_INCLUDES:%=$(SMPATH)/public/%)
IN_TARGET_INCLUDES = $(TARGET_SRC_PATH)/.

WARN_INCLUDES = $(IN_TARGET_INCLUDES)
NO_WARN_INCLUDES = $(SOURCEMOD_INCLUDES)

CPP_FLAGS += -std=c++1z
CPP_FLAGS += -Weverything -Wno-padded # -Wno-float-equal
CPP_FLAGS += -Wno-c++98-compat -Wno-c++98-c++11-compat -Wno-c++98-c++11-compat-pedantic
CPP_FLAGS += -Wno-c++98-compat-pedantic -Wno-c99-extensions -Wno-c++98-c++11-c++14-compat
CPP_FLAGS += -fcolor-diagnostics

LINK_SRCS = $(CPP_SRCS:%=$(TARGET_BLD_PATH)/cpp_o/%.o)
LINK_SRCS += $(BOOSTPATH)

LINK_FLAGS +=


all: $(INCPATH) $(LIBPATH)
.DEFAULT_GOAL = all


define BUILD_CPP_RULE
$(TARGET_BLD_PATH)/cpp_o/$(1).o: $(TARGET_SRC_PATH)/$(1).cpp
	mkdir -p $$(dir $$@)
	$(COMPILER) $(CPP_FLAGS) $(NO_WARN_INCLUDES:%=-isystem%) $(WARN_INCLUDES:%=-I%) -o $$@ -c $$^
endef

$(foreach x,$(CPP_SRCS),$(eval $(call BUILD_CPP_RULE,$(x))))

$(TARGET_BLD_PATH)/cpp_bin/$(LIBFILENAME): $(LINK_SRCS)
	mkdir -p $(dir $@)
	$(LINKER) $(LINK_FLAGS) -o $@ $^


$(INCPATH): $(TARGET_SRC_PATH)/socket.inc
	mkdir -p $(dir $@)
	ln -sf $(shell pwd)/$^ $@

$(LIBPATH): $(TARGET_BLD_PATH)/cpp_bin/$(LIBFILENAME)
	mkdir -p $(dir $@)
	ln -sf $(shell pwd)/$^ $@
