

CC=gcc
OBJF= ./obj/
CFLAGS = -pthread -I./include
objects=  $(OBJF)main.o   $(OBJF)socket.o


$(OBJF)%.o:src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(objects)
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	rm -f  $(OBJF)/*.o
	rm -f server