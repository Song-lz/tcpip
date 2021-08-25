obj=$(patsubst %.c, %.o, $(wildcard *.c))
ARFLAGS = -lpthread
build:$(obj)
	$(CC) $^ -o $@ $(ARFLAGS)

.PHONY:clean
clean:
	$(RM) -rf $(obj)
