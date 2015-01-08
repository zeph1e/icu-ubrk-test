# Makefile template for small or test projects
#
# Copyright(C) 2013 Yunsik Jang <doomsday@kldp.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


# If you use custom toolchain specify it
TOOLCHAIN_PATH :=
TOOLCHAIN_PREFIX :=

ifneq ($(strip $(TOOLCHAIN_PATH)),)
CC := $(TOOLCHAIN_PATH)/bin/$(TOOLCHAIN_PREFIX)$(CC)
CXX := $(TOOLCHAIN_PATH)/bin/$(TOOLCHAIN_PREFIX)$(CXX)
LD := $(TOOLCHAIN_PATH)/bin/$(TOOLCHAIN_PREFIX)$(LD)
endif

# specify target binary names:
#  the source filename which have main function should be same to target name
#  (with file extenstion).
TARGETS := \
	ubrk

# specify objects being referenced from $(TARGETS)
COMMONOBJ := \

# specify package name being known to pkg-config
PKG_DEPENDENCIES := icu-uc icu-io icu-lx icu-i18n
# if pkg-config path is differ from default one. give it here
PKG_CONFIG_PATH :=

PKG_CFLAGS := \
	$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
		 $(patsubst %,pkg-config --cflags %,$(PKG_DEPENDENCIES)))
PKG_LDFLAGS := \
	$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
		$(patsubst %,pkg-config --libs %,$(PKG_DEPENDENCIES)))

# write some include and lib flags
INCLUDES :=

LIBS :=

# some other compiler/linker flags here
CFLAGS := -g

CXXFLAGS :=

LDFLAGS :=


all: $(TARGETS)

%.o:%.c
	$(CC) -c $< $(INCLUDES) $(PKG_CFLAGS) $(CFLAGS)

%.o:%.cpp
	$(CXX) -c $< $(INCLUDES) $(PKG_CFLAGS) $(CXXFLAGS)


$(TARGETS) : $(patsubst %,%.o,$(TARGETS))
	$(CXX) -o $@ $@.o $(COMMONOBJ) $(LDFLAGS) $(PKG_LDFLAGS)

clean:
	rm -f *.o $(TARGETS)
