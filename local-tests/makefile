default: colortest.ppm

colortest: colortest.cpp
	g++ colortest.cpp -o colortest

colortest.ppm: colortest
	./colortest > colortest.ppm

clean:
	-rm -f colortest
	-rm -f colortest.ppm