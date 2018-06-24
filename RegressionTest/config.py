import json
import os.path
import cmdlinewrapper
from performanceregressionconfig import PerformanceRegressionConfig


class ConfigError(Exception):
    def __init__(self, message):
        self.message = message

def buildCmd(testcaseSet):
    cmds = [];
    for testcase in testcaseSet:
        cmd = cmdlinewrapper.DepthmapCmd()
        cmd.infile = testcase["infile"]
        cmd.outfile = testcase["outfile"]
        cmd.mode = testcase["mode"]
        if "simple" in testcase and not testcase["simple"]  == "false":
            cmd.simpleMode = True
        cmd.extraArgs = testcase.get("extraArgs", {})
        cmds.append(cmd)
    return cmds

class RegressionConfig():
    def __init__(self, filename):
        with open(filename, "r") as f:
            config = json.load(f)
        configdir = os.path.dirname(filename)
        self.rundir = config["rundir"]
        self.basebinlocation = config["basebinlocation"]
        self.testbinlocation = config["testbinlocation"]
        self.performanceRegression = PerformanceRegressionConfig(config.get("performance", None))
        self.testcases = {}
        for (name, tc) in config["testcases"].items():
            self.testcases[name] = buildCmd(tc)


