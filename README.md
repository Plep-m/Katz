# Katz - C API Backend with Hot Reloading

Katz is a lightweight C API backend designed for building modular and extensible web services. It supports hot reloading of routes, allowing you to update your API without restarting the server. This makes it ideal for development environments where rapid iteration is required.

---

## Features

- **Hot Reloading**: Routes can be updated dynamically without restarting the server.
- **Modular Design**: Routes are compiled as shared libraries (`.so` files) and loaded at runtime.
- **HTTP Methods**: Supports `GET`, `POST`, `PUT`, `PATCH` and `DELETE` requests out of the box.
- **Thread-Safe**: Uses read-write locks to ensure thread safety during route reloading.
- **Lightweight**: Built on top of the [GNU libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) library.

---

## Demo project  : 
- **https://github.com/Plep-m/katz_demo** 

## Usage

```bash
  clone this repo
  mkdir build && cd build && cmake .. && make && sudo make install
```
This will install the libkatz.so shared library to /usr/local/lib and the public headers to /usr/local/include/katz.
now you can </br>
```c
  #include "katz/katz.h"
  #include <unistd.h>

  int main() {
      if (!katz_init("../config/config.json")) {
          return 1;
      }

      katz_start();

      while (1) {
          sleep(1);
      }

      katz_stop();
      katz_cleanup();
      return 0;
  }
```
