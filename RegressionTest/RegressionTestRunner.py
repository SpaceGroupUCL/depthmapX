import runhelpers
import config
import depthmaprunner
import performancerunner
import os
import sys

defaultConfigFile = "regressionconfig.json"

class RegressionTestRunner():
    def __init__(self, configfile, runfunc):
        self.config = config.RegressionConfig(configfile)
        self.testBinary = runhelpers.getTestExecutable(self.config.testbinlocation)
        print("Binary under test is " + self.testBinary)
        self.baseBinary = runhelpers.getExecutable(self.config.basebinlocation)
        print("Baseline binary is " + self.baseBinary)
        self.runfunc = runfunc

    def run(self):
        if not os.path.exists(self.config.rundir):
            os.makedirs(self.config.rundir)
        if self.config.performanceRegression.enabled:
            print("Performance regression runs enabled")
            runner = performancerunner.PerformanceRunner(self.runfunc, self.baseBinary, self.testBinary, self.config.rundir,self.config.performanceRegression )
        else:
            print("Default regression runs - no performance")
            runner = depthmaprunner.DepthmapRegressionRunner( self.runfunc, self.baseBinary, self.testBinary, self.config.rundir )

        good = True
        for name, case in self.config.testcases.items():
            print("Running test case " + name)
            success, output = runner.runTestCase(name, case)
            if not success:
                good = False
                print ("Failed:\n" + output)
            else:
                print("ok")
        return good        

if __name__ == "__main__":
    print("Starting up RegressionTestRunner")
    configFile = defaultConfigFile
    if len(sys.argv) == 2:
        configFile = sys.argv[1]
    print("Config file in use is: " + configFile)
    r = RegressionTestRunner(configFile, runhelpers.runExecutable)
    print("Setup complete, starting run")
    if not r.run():
        exit(-1)

