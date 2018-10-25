CC = gcc
CFLAGS = -Wall -O2

OBJS = madasd.o utility.o commands.o

TARGET = madasd

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) 

