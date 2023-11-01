# clap

CGI Lab AlphaZero Platform


## Install

### PyTorch with c++11 ABI

#### Install PyTorch Dependencies
```bash
conda create -n clap python=3.8
conda activate clap
conda install numpy ninja pyyaml mkl mkl-include setuptools cmake cffi typing_extensions future six requests dataclasses
# Add LAPACK support for the GPU if needed
conda install -c pytorch magma-cuda110  # or [ magma-cuda112 | magma-cuda100 | magma-cuda92 ] depending on your cuda version
```

#### Get the PyTorch Source
```bash
git clone --recursive https://github.com/pytorch/pytorch -b 1.7
cd pytorch
```

#### Install PyTorch
```bash
export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
GLIBCXX_USE_CXX11_ABI=1 python setup.py install
```

### Build CLAP
#### Get the CLAP Source
```bash
git clone --recursive https://github.com/LiuLiangFu/clap
cd clap
```
#### Install CLAP Dependencies
```bash
conda install pybind11 protobuf
pip install tensorboard zstandard psutil zmq clang-format
```
#### Build
```bash
pip install -e .
```
