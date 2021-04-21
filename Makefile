CC=gcc
CFLAGS=-O3 -march=native -fopenmp -Wall -Wextra -lX11

i3lock-fancy-rapid: i3lock-fancy-rapid.c
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f i3lock-fancy-rapid
