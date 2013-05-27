all: inject-parser

inject-parser:
	gcc -L/usr/lib -o "X-TIER_linux_parser" parser.c -lelf

clean:
	rm X-TIER_linux_parser
