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

## Usage

```bash
  docker compose build
  docker compose up
```