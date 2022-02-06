# debrid-scripts

Collection of scripts used to bypass 1Fichier time restrictions (PoC).

## debrid.sh

This script depends on `cURL`, `html-xml-utils` and `xdg-utils`. On Ubuntu, these dependencies
can be installed by running the following command:
```$ sudo apt install curl html-xml-utils xdg-utils```

Then, run `debrid.sh` using:
```
$ chmod +x debrid.sh
$ ./debrid.sh [url] ... 
```

## debrid.c

A C version of debrid.sh that should run on Unix
and Windows. `libcurl` is the only dependency.
