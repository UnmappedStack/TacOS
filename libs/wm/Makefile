LIBC = ../../libc
HEADERS = $(LIBC)/include

all:
	gcc -c src/* -o libwm.o -ffreestanding -I $(HEADERS) -Iinclude -static -nostdlib -fno-stack-protector -fno-pie -g -mno-tls-direct-seg-refs
