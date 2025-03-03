default: oamtest.ppm

oamtest: oamtest.cpp ../nes_ppu.h
	g++ oamtest.cpp -o oamtest

oamtest.ppm: oamtest
	./oamtest

nametabletest: nametabletest.cpp ../nes_ppu.h
	g++ nametabletest.cpp -o nametabletest

nametabletest001.ppm: nametabletest
	./nametabletest

nametabletest.mp4: nametabletest001.ppm
	ffmpeg -r 10 -f image2 -s 256x240 -i nametabletest%03d.ppm -vcodec libx264 -crf 1  -pix_fmt yuv420p nametabletest.mp4

aputest: aputest.cpp ../nes_apu.h
	g++ aputest.cpp -o aputest

clean:
	-rm -f oamtest
	-rm -f nametabletest
	-rm -f aputest
	-rm -f *.ppm
	-rm -f *.mp4
	-rm -f *.wav
