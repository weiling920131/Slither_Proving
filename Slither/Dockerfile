FROM nvidia/cuda:11.0-cudnn8-devel-ubuntu18.04 AS build
RUN \
  apt-get update && \
  apt-get install -y wget git && \
  wget -qO /opt/miniconda3.sh 'https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh' && \
  chmod a+x /opt/miniconda3.sh && \
  git clone -b v1.6.0 --depth 1 --recurse-submodules --shallow-submodules https://github.com/pytorch/pytorch.git /usr/local/src/pytorch
RUN \
  /opt/miniconda3.sh -b -p /opt/conda && \
  /opt/conda/bin/conda init bash
RUN \
  bash -ic 'conda install -y numpy ninja pyyaml mkl mkl-include setuptools cmake cffi typing_extensions future six requests dataclasses protobuf pyzmq' && \
  bash -ic 'conda install -y -c pytorch magma-cuda110'
RUN \
  cd /usr/local/src/pytorch && \
  bash -ic 'env PYTORCH_BUILD_VERSION=1.6.0 PYTORCH_BUILD_NUMBER=1 USE_DISTRIBUTED=0 BUILD_BINARY=ON _GLIBCXX_USE_CXX11_ABI=1 CMAKE_PREFIX_PATH=$CONDA_PREFIX python setup.py install'

FROM nvidia/cuda:11.0-cudnn8-devel-ubuntu18.04 AS final
COPY --from=build /opt/conda /opt/conda
RUN /opt/conda/bin/conda init bash
