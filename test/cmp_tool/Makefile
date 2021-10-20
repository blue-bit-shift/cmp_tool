CC               = gcc
SOURCEDIR	 =
INCLUDES         = -I../../include
PATH            +=
CFLAGS          := -O0 -W -Wall -Wextra -std=gnu99 -Werror -pedantic -g3\
								-fprofile-arcs -ftest-coverage\
								-fanalyzer
CPPFLAGS        := -D__MAIN__ $(INCLUDES) -I$(SOURCEDIR)
LDFLAGS         := -lcunit
SOURCES         := $(wildcard *.c) ../../lib/cmp_tool_lib.c ../../lib/cmp_support.c
OBJECTS         := $(patsubst %.c, $(BUILDDIR)/%.o, $(subst $(SOURCEDIR)/,, $(SOURCES)))
TARGET          := test_cmp_tool


all: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $(TARGET)

coverage: all
	./$(TARGET)
	lcov --rc lcov_branch_coverage=1 --capture --directory ./ --output-file coverage.info
	genhtml --rc lcov_branch_coverage=1 --branch-coverage coverage.info --output-directory out
	#firefox out/index.html


clean:
	rm -r $(TARGET) *.gcno *.gcda coverage.info out/ tmp_stderr.log
