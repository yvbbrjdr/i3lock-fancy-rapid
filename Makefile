CC=gcc
CFLAGS=-O3 -fopenmp -Wall -Wextra -lX11
PROG=i3lock-fancy-rapid
BINDIR=/usr/bin

all: $(PROG) install

$(PROG): $(PROG).c
	$(CC) $^ $(CFLAGS) -o $@

install:
	@install -Dm755 $(PROG) -t $(BINDIR)

clean:
	rm -f $(PROG)
	rm -f /$(BINDIR)/$(PROG)
