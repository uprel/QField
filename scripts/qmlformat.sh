#!/bin/bash

docker run -i --rm -v $(pwd):/usr/src sandroandrade/qt6 /usr/src/scripts/qmlformat-wrapper.sh $@
