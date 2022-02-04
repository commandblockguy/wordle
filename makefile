# ----------------------------
# Makefile Options
# ----------------------------

PATH := /home/john/CEdev/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

NAME = WORDLE
ICON = icon.png
DESCRIPTION = "Wordle"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz -DCOMMIT=\"$(shell git rev-parse --short HEAD)\"
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
