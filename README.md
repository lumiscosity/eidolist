# Eidolist
A patch merger for RPG Maker 2000/3 projects, using patches provided by [Mossball](https://github.com/lumiscosity/mossball) as a base. While it is primarily intended for use in the Collective Unconscious project, you may use it for your own projects as well.

## Building
Requires `Qt6` and uses CMake for building. `liblcf` will be built as part of the process.

```
git clone https://github.com/lumiscosity/eidolist
cd eidolist
git submodule update --init
cmake -B builddir
cmake --build builddir
```

## License
Eidolist is free software licensed under GPLv3.
