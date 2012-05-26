FLAGS= -I. 
LIBS=libs/crypt/{base64,md5}.c
FILE=ipoirc.c
BIN=ipoirc
BINDEMO=demo

$(BIN): 
	$(CC) $(CFLAGS) -o $(BIN) $(FILE) $(FLAGS) $(LIBS)
clean:
	rm -f $(BIN) $(BINDEMO)
$(BINDEMO):
	$(CC) -O0 -g -DNDEBUG -Wall -DMX=1024 -o $(BINDEMO) libsdemo.c $(FLAGS) libs/*.c libs/*/*.c
