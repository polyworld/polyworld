ALL_PRODUCTS = genetrace

EXCEPTIONS = 

libshared_dir = $(src)/shared

PRODUCTS := $(filter-out $(EXCEPTIONS), $(ALL_PRODUCTS))

SRCS = $(foreach pattern, $(PRODUCTS), $(pattern).c)

OBJDIR = objs

OBJS = $(foreach pattern, $(PRODUCTS), $(OBJDIR)/$(pattern).o)

LIBSHARED := shared
LIBSHARED_HEADERS = Shared.h ArgParse.h Stream.h

HEADERS = $(wildcard $(libshared_dir)/*.h)

ifeq "CC" "gcc3"
	ISO_CFLAGS = -std=iso9899:1999 -pedantic
endif

CFLAGS = -g -Wall -Werror $(ISO_CFLAGS) -I $(libshared_dir)
LDFLAGS := -g -L$(libshared_dir)
FRAMEWORKS := 

CPPFLAGS = $(CFLAGS)

default: all

all: $(OBJDIR) $(OBJS) $(PRODUCTS)

clean:
	rm -rf $(PRODUCTS)
	rm -rf $(OBJDIR)
	
$(OBJS): $(HEADERS)

$(PRODUCTS): %: $(OBJDIR)/%.o $(libshared_dir)/lib$(LIBSHARED).a
	cc $(LDFLAGS) -o $@ $< -lshared $(FRAMEWORKS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cc
	$(C++) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.m
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR):
	mkdir -p $(OBJDIR)
