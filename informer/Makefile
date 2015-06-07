CC = arm-linux-gcc
informer:version_parse.c
	${CC} version_parse.c -o version_parse -ljson -L/usr/local/arm/json-c-0.9/lib -L/usr/local/arm/sqlite-3.7.14/lib -I/usr/local/arm/json-c-0.9/include/json -I/usr/local/arm/sqlite-3.7.14/include

.PHONY : clean
clean :
	-rm -f version_parse
