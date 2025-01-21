default: oamtest.ppm

oamtest: oamtest.cpp
	g++ oamtest.cpp -o oamtest

oamtest.ppm: oamtest
	./oamtest

nametabletest: nametabletest.cpp
	g++ nametabletest.cpp -o nametabletest

nametabletest.ppm: nametabletest
	./nametabletest

clean:
	-rm -f oamtest
	-rm -f nametabletest
	-rm -f *.ppm