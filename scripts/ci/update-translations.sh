#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
source ${DIR}/../version_number.sh

lupdate -recursive ${DIR}/../.. -ts ${DIR}/../../i18n/qfield_en.ts

echo === -1 
echo ${CI_BUILD_DIR}
echo ${DIR}
ROOTDIR=${DIR}/../..
echo $ROOTDIR

ls -l ${CI_BUILD_DIR}/i18n/qfield_en.ts
ls -l ${CI_BUILD_DIR}/i18n/qfield_bg.ts

echo ==================1
grep Trans-test-ta ${CI_BUILD_DIR}/i18n/qfield_en.ts
echo ==================1

echo ==================2
grep Trans-test-ta ${CI_BUILD_DIR}/i18n/qfield_bg.ts
echo ==================2


# release only if the branch is master
if [[ ${CI_BRANCH} = master ]]; then
  tx push --source
fi

# release only if the branch is master
if [[ ${CI_BRANCH} = trans-test ]]; then
  tx push --source
fi
