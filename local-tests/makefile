default: oamtest.ppm

oamtest: oamtest.cpp
	g++ oamtest.cpp -o oamtest

oamtest.ppm: oamtest
	./oamtest

clean:
	-rm -f colortest
	-rm -f colortest.ppm