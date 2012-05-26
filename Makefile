FLAGS= -I. 
LIBS=libs/crypt/{base64,md5}.c
FILE=ipoirc.c
BIN=ipoirc

$(BIN): 
	$(CC) $(CFLAGS) -o $(BIN) $(FILE) $(FLAGS) $(LIBS)
clean:
	rm -f $(BIN)
