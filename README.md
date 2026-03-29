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

``` bash
cmake --build build -t format # utilise automatiquement .clang-format
cmake --build build -t tidy # utilise automatiquement .clang-tidy
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

## pre-commit

.git/hooks/pre-commit

``` bash
#!/bin/sh
set -e

echo "[pre-commit] clang-format"
cmake --build build -t format

echo "[pre-commit] clang-tidy"
cmake --build build -t tidy
```

``` bash
chmod +x .git/hooks/pre-commit
```

## workflow

``` bash
cmake -S . -B build
cmake --build build -j

# au quotidien
cmake --build build -t format
cmake --build build -t tidy
```
