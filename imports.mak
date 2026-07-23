#
# Set location of various cgtools
#
# These variables can be set here or on the command line.  Paths must not
# have spaces.
#

SYSCONFIG_TOOL         ?= /Applications/ti/sysconfig_1.26.3/sysconfig_cli.sh
SIMPLELINK_WIFI_TOOLBOX_INSTALL_DIR ?= /Applications/ti/simplelink_wifi_toolbox_macos_4_1_11

CMAKE                  ?= /opt/homebrew/bin/cmake
PYTHON                 ?= python3

TICLANG_ARMCOMPILER    ?= /Users/x/ti/ccs2041/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS
GCC_ARMCOMPILER        ?= /Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi
#
#
# Do Not change the variables below these comments
#




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
