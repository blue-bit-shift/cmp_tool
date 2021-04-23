CC               = gcc
SOURCEDIR	       = lib
INCLUDEDIR       = include
BUILDDIR         = ./
CFLAGS          := -Wall -Wextra -std=gnu99 -pedantic -Wshadow
RELCFLAGS       := -O2 # Release flags
DBCFLAGS        := -O0 -g3 #debug flags
CPPFLAGS        := -I $(INCLUDEDIR)
LDFLAGS         := -lm
SOURCES         := cmp_tool.c \
		    $(SOURCEDIR)/rmap.c \
		    $(SOURCEDIR)/rdcu_ctrl.c \
		    $(SOURCEDIR)/rdcu_cmd.c \
		    $(SOURCEDIR)/rdcu_rmap.c \
		    $(SOURCEDIR)/cmp_support.c \
		    $(SOURCEDIR)/cmp_data_types.c \
		    $(SOURCEDIR)/cmp_rdcu.c \
		    $(SOURCEDIR)/cmp_icu.c \
		    $(SOURCEDIR)/decmp.c \
		    $(SOURCEDIR)/rdcu_pkt_to_file.c \
		    $(SOURCEDIR)/tool_lib.c
TARGET          := cmp_tool

DEBUG?=1
ifeq  "$(shell expr $(DEBUG) \> 1)" "1"
	    CFLAGS += -DDEBUGLEVEL=$(DEBUG)
else
	    CFLAGS += -DDEBUGLEVEL=1
endif


all: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RELCFLAGS) $^ -o $(TARGET) $(LDFLAGS)

debug: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DBCFLAGS) $^ -o $(TARGET) $(LDFLAGS)
