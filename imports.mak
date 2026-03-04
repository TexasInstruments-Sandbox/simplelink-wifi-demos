#
# Set location of various cgtools
#
# These variables can be set here or on the command line.  Paths must not
# have spaces.
#

SYSCONFIG_TOOL         ?= /home/username/ti/ccs2010/ccs/utils/sysconfig_1.26.0/sysconfig_cli.sh

CMAKE                  ?= /home/username/cmake-3.21.3/bin/cmake
PYTHON                 ?= python3

TICLANG_ARMCOMPILER    ?= /home/username/ti/ccs2010/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS
GCC_ARMCOMPILER        ?= /home/username/arm-none-eabi-gcc/12.3.Rel1-0

#
# Do Not change the variables below these comments
#

SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR = resources/simplelink-wifi-sdk


ifeq ("$(SHELL)","sh.exe")
    # for Windows/DOS shell

    # Note that sadly Windows' del command can't handle forward slashes so the
    # SLASH_FIXUP function is used to insulate portable makefiles from that.
    # Typically this is only used for clean goals (where 'del' is used), and
    # only when removing files specified with full, forward-slash-containing
    # paths.
    SLASH_FIXUP = $(subst /,\,$1)

    RM      = del
    RMDIR   = -rmdir /S /Q
    DEVNULL = NUL
    ECHOBLANKLINE = @cmd /c echo.
else
    # for Linux-like shells
    SLASH_FIXUP = $1

    RM      = rm -f
    RMDIR   = rm -rf
    DEVNULL = /dev/null
    ECHOBLANKLINE = echo
endif
