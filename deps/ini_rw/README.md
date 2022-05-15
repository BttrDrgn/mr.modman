
# ini_rw
A lightweight ANSI C library for working with .ini config files

## Usage
The files **[ini_rw.c](src/ini_rw.c?raw=1)** and **[ini_rw.h](src/ini_rw.h?raw=1)**
should be dropped into an existing project.

The library has support for sections, comment lines and quoted string values
(with escapes). Unquoted values and keys are trimmed of whitespace when loaded.

```ini
; last modified 1 April 2001 by John Doe
[owner]
name = John Doe
organization = Acme Widgets Inc.
phone = (000) 5551234

[database]
; use IP address in case network name resolution is not working
server = 192.0.2.62
port = 143
file = "payroll.dat"
```

An ini file can be loaded into memory by using the `ini_load()` function.
`NULL` is returned if the file cannot be loaded.
```c
ini_t *config = ini_load("config.ini");
```

Alternatively, create an ini data structure from the data previously read from
a file, if you need control over how the file is loaded.
```c
char *data;
size_t sz;
// read data, put size of data in sz...
ini_t *config = ini_create(data, sz);
```

The `ini_t*` object can be later written back to a file using the `ini_save()`
function. `1` is returned if writing succeeded, otherwise `0`.
```c
ini_t *ini;
// create ini...
ini_save(ini, "config.ini");
```

The contents of an `ini_t*` object, in a string of the same format that would
be stored in an actual .ini file, can be obtained by using the `ini_tostring()`
function. With the ini data in hand, you can write it to a file however you
want. The string must be freed with the C `free()` function, when you're done
with it.
```c
ini_t *ini;
// create ini...
char *ini_str = ini_tostring(ini);
printf("%s\n", ini_str);
free(ini_str);
```

There are two functions for retrieving values: The first is `ini_get()`.  Given
a section and a key the corresponding value is returned if it exists.  If the
`section` argument is `NULL` then all sections are searched.
```c
const char *name = ini_get(config, "owner", "name");
if (name) {
  printf("name: %s\n", name);
}
```

The second, `ini_sget()`, takes the same arguments as `ini_get()` with the
addition of a scanf format string and the format string's associated variable
arguments list.
```c
int area_code, phone_number;

ini_sget(config, "owner", "phone", "(%d) %d", &area_code, &phone_number);

printf("phone: (%d) %d\n", area_code, phone_number);

const char *server;
int port = 80;

server = ini_get(config, "database", "server");
if (!server) {
  server = "default";
}
ini_sget(config, "database", "port", "%d", &port);

printf("server: %s:%d\n", server, port);
```

There are two functions for adding or changing values: The first is
`ini_set()`.  Given a section and a key the corresponding value is stored,
replacing the previous value if it already existed. If you set a key to `NULL`
or the empty string, the key is deleted.  After calling a set function, strings
returned from `ini_get()` are invalidated, because the set functions change the
memory location of the memory where the strings are stored. So, when using set
functions, keep that in mind and don't keep hold of strings from `ini_get()`,
just make copies.
```c
ini_set(config, "owner", "name", "Jane Doe");
```

The second, `ini_pset()`, takes the same arguments as `ini_set()` with the
addition of a printf format string and the format string's associated variable
arguments list.
```c
ini_pset(config, "database", "server", "%s", "192.0.2.62");
ini_pset(config, "database", "port", "%d", 80);
ini_pset(config, "owner", "phone", "(%d) %d", 000, 5551234);
```

The `ini_free()` function is used to free the memory used by the `ini_t*`
object when you are done with it. Calling this function invalidates all string
pointers returned by the library.
```c
ini_free(config);
```

Some additional functions are available, that might be useful: `ini_merge()`
and `ini_copy()`.

`ini_merge()` copies all the entries from one ini object into another,
overwriting the preexisting keys in the process.
```c
ini_t *config;
ini_t *extra_settings;
// merge extra_settings into the config
ini_merge(config, extra_settings);
```

`ini_copy()` creates a full copy of an `ini_t*` object. The copy must still be
freed with `ini_free()`.
```c
ini_t *ini = ini_load("config.ini");
ini_t *copy_of_ini = ini_copy(ini);
ini_free(ini);
ini_free(copy_of_ini);
```

## License
This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
