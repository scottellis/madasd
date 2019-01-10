CC = gcc
CFLAGS = -O2 -Wall
LIBS = -lpthread

OBJS = ads127x.o madasd.o utility.o

TARGET = madasd

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) 

