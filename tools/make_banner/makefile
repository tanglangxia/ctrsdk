all:
	gcc -std=c99 main.c icn.c bnr.c utils.c yaml.c -o bin/make_banner

banner:
	bin\make_banner -v -g test\graphics.cbmd -a test\audio.cwav --makebnr bin\test.bnr

icon:
	bin\make_banner -v -b test\test.bsf -s test\small_icon.ctpk -l test\large_icon.ctpk --makeicn bin\out.icn
