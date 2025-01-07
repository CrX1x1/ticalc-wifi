# ----------------------------
# Makefile Options
# ----------------------------

NAME = WIFI
DESCRIPTION = "sigma rizz wifi"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)