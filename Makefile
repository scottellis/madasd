CC = gcc
CFLAGS = -Wall -O2
LIBS = -lpthread

OBJS = ads127x.o madasd.o utility.o

TARGET = madasd

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) 

