# Runtime and STD

The runtime of ymir, link directly at compilation time
The runtime is in two parts, the std and the real runtime.
Complete compilation of the runtime is made in the Ymir-Docker repository, for example : [Dockerfile](https://github.com/GNU-Ymir/Ymir-Docker/blob/master/builder_image/9/amd64/runtime/Dockerfile)

## Local installation for tests and debug

If you have compiled gyc yourself on your computer, and just want to
test an updated std. Before executed the following script, you have to
install gyc locally, following the instruction given in
[gymir/Readme](https://github.com/GNU-Ymir/gymir/blob/master/README.md)

```bash
cd {install_dir}
git clone https://github.com/GNU-Ymir/yruntime.git yruntime
cd {install_dir}/yruntime/midgard

# install the header files (that can be imported in ymir within: core::, std::, etc::)
# requires a locally installed gyc  
sudo ./install

# compile the std, require a locally installed gyc
mkdir .build
cd .build
cmake ..
make
sudo make install # install the .so in /usr/lib

cd {install_dir}/yruntime/runtime
mkdir .build
cd .build
cmake ..
make
sudo make install # install the .so in /usr/lib
```

## Create a release

The repository [Ymir-Docker](https://github.com/GNU-Ymir/Ymir-Docker) is the repository used to create releases of the runtime.