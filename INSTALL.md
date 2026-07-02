Building Vexta Core
===================

See the documentation in `doc/build-*.md` for platform-specific build instructions.

Common build guides include:

- `doc/build-unix.md`
- `doc/build-osx.md`
- `doc/build-windows.md`

Typical Unix build:

    ./autogen.sh
    ./configure
    make -j$(nproc)

Run tests with:

    make check
