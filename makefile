# ----------------------------
# Makefile Options
# ----------------------------

NAME = WORDLE
ICON = icon.png
DESCRIPTION = "Wordle"
COMPRESSED = YES
ARCHIVED = YES

WORDLIST ?= 1

CFLAGS = -Wall -Wextra -Oz -DCOMMIT=\"$(shell git rev-parse --short HEAD)\" -DSELLOUT_MODE=$(SELLOUT)
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
