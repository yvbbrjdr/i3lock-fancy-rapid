# i3lock-fancy-rapid

A faster implementation of [i3lock-fancy](https://github.com/meskarune/i3lock-fancy). It is blazing fast and provides a fully configurable box blur. It uses linear-time box blur and accelerates using OpenMP.

## Demo

![](demo.png)

## Build

Make sure you have installed the following dependencies and run
```bash
git clone https://github.com/yvbbrjdr/i3lock-fancy-rapid
cd i3lock-fancy-rapid
make
```

## Usage

**Make sure `i3lock-color` has been installed in `PATH` dir. `i3lock-color` create the executable file named `i3lock` which is same with `i3lock` (https://github.com/i3/i3lock) project.**

**So make sure `i3lock-color` is running instead of `i3lock`.**

```bash
i3lock-fancy-rapid radius times [OPTIONS]
```

- `radius` is the kernel radius of box blur
- `times` is the number of times box blur is applied (`pixel` for pixelation)
- `OPTIONS` will be passed to `i3lock`

The above demo uses `i3lock-fancy-rapid 5 3`.

## Dependencies

- [libX11](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html) for screenshot
- [i3lock-color](https://github.com/Raymo111/i3lock-color) master for locking with enhance color config.

## License

[BSD 3-Clause](LICENSE)
