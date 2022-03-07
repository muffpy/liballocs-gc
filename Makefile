LD_FLAGS += -lunwind-x86_64 -lunwind

LIBALLOCS_PATH = usr/local/src/liballocs
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/src/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/liballocstool/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/contrib/librunt/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/include/

# test : test.c
# 	allocscc ${LD_FLAGS}  -I/usr/local/src/liballocs/include/ -I/usr/local/src/liballocs/src/ -I/usr/local/src/liballocs/contrib/liballocstool/include/ -I/usr/local/src/liballocs/contrib/libsystrap/contrib/librunt/include/ -I/usr/local/src/liballocs/contrib/libsystrap/include/ -o test test.c

test : test.c
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o test test.c

run: test
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./test