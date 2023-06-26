# sdbsearch

simple tool to read records from an Intermediate table into a prefix tree for searching number.
Also has an option to export the record in a csv file.

## compile
just call `./make` or compile directly with `gcc -o sdbsearch sdbsearch.c`

## usage
```
sdbsearch V0.1

usage: sdbserch [OPTIONS] [NUMBER 1] [NUMBER 2] ... [NUMBER n]
options:
        -h, --help       print this help
        -a, --all        show all matching numbers
        -n, --nofilter   do not filter 43 and length 20
        -e, --export     write all records in csv format to stdout
        -v, --verbose    verbose output

```
