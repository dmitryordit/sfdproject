#!/bin/bash

# find . -name 'Makefile*' |xargs -I "{}" rm -v "{}"

find . -name '*.o' |xargs -I "{}" rm -v "{}"
find . -name 'moc_*' |xargs -I "{}" rm -v "{}"
find . -name '*.pro.user*' |xargs -I "{}" rm -v "{}"
find . -name 'build-*' |xargs -I "{}" rm -r "{}"

