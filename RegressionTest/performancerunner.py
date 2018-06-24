from statistics import mean
from collections import OrderedDict

import depthmaprunner
import os
import csv

import runhelpers
from performanceregressionconfig import PerformanceRegressionConfig

def checkPerformance(baseFile, testFile, relativeThreshold, absoluteThreshold):
    """
    Check the performance of 2 depthmap runs against each other
    This function expects the timing from a base and test run and  parses them
    as CSV. For now, it expects the entries to be the same. It will return an
    error message if
    * one or both of the files are missing
    * the number of lines or the labels don't match
    * the test run is more than 5 seconds or 5% slower than the baseline
      (whatever is greater)
    """
    if not os.path.exists(baseFile):
        return "Base performance timing file {0} is missing".format(baseFile)
    if not os.path.exists(testFile):
        return "Test performance timing file {0} is missing".format(testFile)
    with open(baseFile) as baseHandle, open(testFile) as testHandle:
        baseReader = csv.DictReader(baseHandle)
        testReader = csv.DictReader(testHandle)

        baseDone = False
        testDone = False

        while True:
            try:
                baseLine = next(baseReader)
            except StopIteration:
                baseDone = True

            try:
                testLine = next(testReader)
            except StopIteration:
                testDone = True

            if baseDone and testDone:
                return ""
            if baseDone and not testDone:
                return "baseline performance file {0} has fewer lines than the test one {1}".format(baseFile, testFile)
            if testDone and not baseDone:
                return "baseline performance file {0} has more lines than the test one {1}".format(baseFile, testFile)

            if not baseLine["action"] == testLine["action"]:
                return "performance line mismatch: base '{0}', test '{1}'".format(baseLine["action"], testLine["action"])

            baseTime = float(baseLine["average"])
            testTime = float(testLine["average"])

            allowance = max(absoluteThreshold, baseTime * relativeThreshold / 100 )
            if testTime > baseTime + allowance:
                return "Performance regression: {0} took {1}s instead of {2}s".format(baseLine["action"], testLine["average"], baseLine["average"])

def aggregatePerformanceStats(dir, numRuns, numCmds, filenameTemplate ):
    data = OrderedDict()
    totalValues = []
    for i in range(numRuns):
        for j in range(numCmds):
            with open(os.path.join(dir, filenameTemplate.format(i, j)), "r") as f:
                reader = csv.DictReader(f)
                total = 0
                for line in reader:
                    if not line["action"] in data:
                        data[line["action"]] = []
                    data[line["action"]].append(float(line["duration"]))
                    total = total + float(line["duration"])
                totalValues.append(total)
    data["total"] = totalValues

    outputFile =os.path.join(dir, filenameTemplate.format("", "all"))
    with open(outputFile, "w+") as f:
        writer = csv.DictWriter(f, ["action", "min", "max", "average"])
        writer.writeheader()
        for key, val in data.items():
            rowDict = {"action": key, "min": min(val), "max": max(val), "average": mean(val)}
            writer.writerow(rowDict)
    return outputFile

class PerformanceRunner(depthmaprunner.DepthmapRegressionRunner):
    def __init__(self, runFunc, baseBinary, testBinary, workingDir,  perfConfig):
        depthmaprunner.DepthmapRegressionRunner.__init__(self,runFunc,baseBinary,testBinary,workingDir)
        self.perfConfig = perfConfig

    def runTestCase(self, name, cmds):
        runhelpers.prepareDirectory(self.makeBaseDir(name))
        runhelpers.prepareDirectory(self.makeTestDir(name))

        nameTemplate = "timings_{0}_{1}.csv"
        for i in range(self.perfConfig.runsPerInstance):
            print ("Running test case {0}, run {1} of {2}".format(name, i, self.perfConfig.runsPerInstance))
            for j in range(len(cmds)):
                cmds[j].timingFile = nameTemplate.format(i,j)
            result, message = self.runTestCaseImpl(name, cmds)
            if not result:
                return (False, "Run {0} failed with message: {1}".format(i, message))

        testFile = aggregatePerformanceStats(self.makeTestDir(name),self.perfConfig.runsPerInstance, len(cmds), nameTemplate)
        baseFile = aggregatePerformanceStats(self.makeBaseDir(name),self.perfConfig.runsPerInstance, len(cmds), nameTemplate)
        message = checkPerformance(testFile, baseFile, self.perfConfig.relativeThresholdInPercent, self.perfConfig.absoluteThresholdInSeconds)
        if message:
            return (False, message)
        return (True, "")