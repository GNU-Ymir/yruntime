# Runtime and STD

The runtime of ymir, link directly at compilation time
The runtime is in two parts, the std and the real runtime.
Complete compilation of the runtime is made in the Ymir-Docker repository, for example : [Dockerfile](https://github.com/GNU-Ymir/Ymir-Docker/blob/master/builder_image/9/amd64/runtime/Dockerfile)

## Compile the runtime

The runtime is developped in C, and in directory `/runtime/`. To compile it :

```bash
cd runtime/
mkdir .build
cd .build
cmake ..
make
make install DESTDIR=/tmp/midgard/
```

## Compile the std

```bash
cd midgard
mkdir .build
cd .build
cmake ..
make
make install DESTDIR=/tmp/midgard/
cd ..
mkdir -p /tmp/midgard/usr/lib/gcc/x86_64-linux-gnu/9/include/ymir/
cp -r core /tmp/midgard/usr/lib/gcc/x86_64-linux-gnu/9/include/ymir/
cp -r etc /tmp/midgard/usr/lib/gcc/x86_64-linux-gnu/9/include/ymir/
cp -r std /tmp/midgard/usr/lib/gcc/x86_64-linux-gnu/9/include/ymir/
```

## Create a deb file

```bash
fpm -s dir -t deb -n libgmidgard-{version} -v {sub-version} -C /tmp/midgard/ \
	-p libgmidgard_VERSION_ARCH.deb \
	-d "libc6 >= {libc6-version}" \
	-d "gcc-{gcc-version}-base >= {gcc-version}" \
	-d "libgcc1" \
	-d "zlib1g-dev" \
	-d "zlib1g >= {zlib1g-version}" \
	usr/lib
```
