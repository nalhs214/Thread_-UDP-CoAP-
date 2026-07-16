CC      = gcc
TARGET  = mqtt_vm
SRCS    = main.c mqtt.c input.c
OBJS    = $(SRCS:.c=.o)
LIBS    = -lmosquitto

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)