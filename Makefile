all : x11stb_image

x11stb_image : x11stb_image.c rawdraw/CNFGFunctions.c
	gcc -o $@ $^ -Irawdraw -Istb -lX11 -lm -Os -s

clean :
	rm -rf x11stb_image

