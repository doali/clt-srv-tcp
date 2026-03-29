# clt-srv-tcp

## Dépendances

``` bash
sudo dnf install -y \
  gcc-c++ \
  cmake \
  boost-devel
```

> Vérification

``` bash
rpm -q boost-devel
```

## Compilation 

```bash
cmake -S . -B build -DNETLIB_USE_BOOST_ASIO=ON -DNETLIB_BUILD_EXAMPLES=ON
cmake --build build -j
```

> Vérification

``` bash
ldd ./build/netlib_echo_client
```

``` text
✅ libboost_system.so
✅ libpthread.so
✅ libstdc++.so
```

## Exécution

```bash
./build/netlib_echo_server
```

```bash
./build/netlib_echo_client
```
