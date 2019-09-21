

CC=gcc
OBJF= ./obj/
CFLAGS = -pthread -I./include -g
objects=  $(OBJF)main.o  $(OBJF)socketmgr.o

$(OBJF)%.o:%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(objects)
	$(CC) -o $@ $^ $(CFLAGS) 
clean:
	rm -f  $(OBJF)/*.o
	rm -f client