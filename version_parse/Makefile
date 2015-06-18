CC = mipsel-openwrt-linux-gcc
version_parse:version_parse.c
	${CC} version_parse.c -o version_parse -ljson -L/usr/local/mips/json-C-0.9/lib -L/usr/local/mips/sqlite-3.7.14/lib -I/usr/local/mips/json-C-0.9/include/json -I/usr/local/mips/sqlite-3.7.14/include

.PHONY : clean
clean :
	-rm -f version_parse
