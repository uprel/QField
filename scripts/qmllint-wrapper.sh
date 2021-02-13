#!/bin/bash

# !!!!! You probably want to call scripts/qmllint.sh instead of this file !!!!!!

# Adds the possibility to run qmllint on stdin if no arguments are given. Otherwise forwards parameters to qmllint.
# Designed to run in a Docker container.

ARGS=""

for arg in "$@"
do
  if [[ $arg =~ .qml$ ]]; then
    ARGS="$ARGS /usr/src/$arg"
  else
    ARGS="$ARGS $arg"
  fi
done

if [ $# -eq 0 ];
then
  cp /dev/stdin /tmp/file.qml
  ARGS="/tmp/file.qml"
fi

/usr/local/Qt-5.15.0/bin/qmllint -U -I/usr/local/Qt-5.15.0/qml -i/usr/local/Qt-5.15.0/qml/QtGraphicalEffects/plugins.qmltypes -i/usr/local/Qt-5.15.0/qml/QtQuick.2/plugins.qmltypes $ARGS
