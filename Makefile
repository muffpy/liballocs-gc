test : test.c
	allocscc -lunwind-x86_64 -lunwind -I/usr/local/src/liballocs/include/ -I/usr/local/src/liballocs/src/ -I/usr/local/src/liballocs/contrib/liballocstool/include/ -I/usr/local/src/liballocs/contrib/libsystrap/contrib/librunt/include/ -I/usr/local/src/liballocs/contrib/libsystrap/include/ -o test test.c

main: test
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./test