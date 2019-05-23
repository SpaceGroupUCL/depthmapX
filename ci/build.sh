#!/bin/sh

source /opt/qt511/bin/qt511-env.sh
if [ ! -d build ]; then
    mkdir build
fi
cd build
# build
echo Building
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . || exit 1
# if succeeds, run unit tests
echo Running unit tests
./cliTest/cliTest && ./GuiUnitTest/GuiUnitTest && ./mgraph440Test/mgraph440Test && ./salaTest/salaTest && ./genlibTest/genlibTest && ./depthmapXTest/depthmapXTest || exit 1
# if that succeeds, run regression tests
echo testing regression test framework
cd ../RegressionTest/test && echo pwd && python3.5 -u test_main.py || exit 1
echo running standard regression tests
cd .. && pwd && python3.5 -u RegressionTestRunner.py || exit 1
echo running agent test
python3.5 -u RegressionTestRunner.py regressionconfig_agents.json || exit 1
