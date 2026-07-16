CC      = gcc
TARGET  = mqtt_rpi
SRCS    = main.c mqtt.c sensor.c command.c
OBJS    = $(SRCS:.c=.o)
LIBS    = -lmosquitto -lcjson

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)