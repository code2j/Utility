일부 라이브러리가 cpp12만 지원하는 경우가 있을거임.
```shell
sudo apt install -y gcc-12 g++-12
export CUDAHOSTCXX=/usr/bin/g++-12
export NVCC_PREPEND_FLAGS='-ccbin /usr/bin/g++-12'
export TORCH_CUDA_ARCH_LIST="8.9"
export CUDAHOSTCXX=/usr/bin/g++-12
```
