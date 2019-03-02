PROG = madasd
SRCS = adsinterface.c \
       madasd.c \
       utility.c

LIBS = -lpthread

OBJS = $(SRCS:.c=.o)

# CC = gcc
# CFLAGS = -O2 -Wall
#CFLAGS = -fsanitize=address -fno-omit-frame-pointer
#CFLAGS = -fsanitize=thread -fno-omit-frame-pointer

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(PROG)

