main.c is the most up to date program.
to compile use:
gcc main.c LMP90100.c ADS1256.c -o ee -lbcm2835 $(pkg-config --libs --cflags libmodbus)