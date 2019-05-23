#!/bin/sh
timestamp=$(date +%Y.%m.%d.%H.%M.%S)

if [ $# -ne 3 ]; then
    echo "Three arguments are required, runner, sync git path and location of server upload php file"
    exit 1
fi

runner=$1
gitpath=$2
uploadpath=$3

cd "$(dirname "$0")"
rm -rf depthmapX/
git clone $gitpath
currentcommit=$(git rev-parse HEAD)
cd depthmapX
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
cd ../RegressionTest
python3 RegressionTestRunner.py  performance_regression.json
cd ../../
mkdir -p runs
cd runs
rundir=run-$timestamp
mkdir $rundir
cp -r ../depthmapX/RegressionTest/rundir/* $rundir/

cd $rundir

params="{\"time\":\"$timestamp\",\"runner\":\"$runner\",\"commit\":\"$currentcommit\",\"tests\":["

counter="0"
for i in $(find . | grep 'timings_[0-9]\+_[0-9]\+\.csv')
do
    newparams=$(csvtool drop 1 $i | csvtool format ',"%(1)":"%(2)"' - | cut -c2-);
    #newparams="${newparams// /_}"
    if [ "$counter" -eq "1" ]; then
         params="${params},";
    fi
    params="${params}{\"file\":\"$i\",\"times\":{$newparams}}"
    counter="1"
done

params="${params}]}"
curl --data "$params" $uploadpath
