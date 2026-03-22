# modc

modc (Modified C) is a custom language modelled after C.
The language is modified to add classes and easier primitives along with containers.

This project is for me to learn how compilers work and to write one.

## Dependencies

This repository vendors some dependencies as Git submodules under `lib/`.

- `lib/libc-additions` — builds a static library archive at `lib/libc-additions/lib/libc-additions.a`

## Clone

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/Shadowmachete/modc.git
cd modc
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

## Build

```bash
make
```

## Clean

Remove only modc build outputs:

```bash
make clean
```

Remove modc outputs **and** submodule build outputs:

```bash
make distclean
```

## Updating the submodule (maintainers)

To update the `libc-additions` submodule pointer:

```bash
git submodule update --remote --merge
git commit -am "Update libc-additions submodule"
```
