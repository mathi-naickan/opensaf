#!/bin/sh
autoreconf -vi

sed -i "s/^INTERNAL_VERSION_ID=.*\$/INTERNAL_VERSION_ID=$(git rev-parse HEAD)/" configure
