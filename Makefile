default_target: all

MAIL_CLIENT_OBJS += \
client.o

MAIL_SERVER_OBJS += \
server.o 

MAIL_CLIENT_EXE += mail_client
MAIL_SERVER_EXE += mail_server

./%.o: ./%.c
	gcc -c $< -o $@

all: mail_client mail_server

mail_client: $(MAIL_CLIENT_OBJS)
	gcc -o $(MAIL_CLIENT_EXE) $(MAIL_CLIENT_OBJS)
	@echo ' '

mail_server: $(MAIL_SERVER_OBJS)
	gcc -o $(MAIL_SERVER_EXE) $(MAIL_SERVER_OBJS)
	@echo ' '

clean:
	@rm -v $(MAIL_CLIENT_OBJS) $(MAIL_SERVER_OBJS)
	@rm -v $(MAIL_CLIENT_EXE) $(MAIL_SERVER_EXE)
	@echo ' '
