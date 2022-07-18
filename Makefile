mcp3008-poll: mcp3008-poll.c
	$(CC) $(CFLAGS) -Ofast mcp3008-poll.c -o mcp3008-poll

clean:
	rm -f mcp3008-poll
