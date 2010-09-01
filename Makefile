
.PHONY: clean

hashy: hashy.c
	$(CC) -g -o hashy hashy.c -levent -ltokyocabinet

clean: 
	rm -f hashy
