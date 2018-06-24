import unittest
from context import config
from disposablefile import DisposableFile
import RegressionTestRunner
import os

def writeConfig(filename, rundir):
    with open(filename, "w") as f:
        f.write('{ "rundir": "'+ rundir +'",\n')
        f.write('   "basebinlocation": "../baselineBinaries",\n')
        f.write('   "testbinlocation": "../..",\n')
        f.write('  "testcases": {\n')
        f.write('      "test1": [{\n')
        f.write('           "infile": "infile.graph",\n')
        f.write('           "outfile": "outfile.graph",\n')
        f.write('           "mode": "VGA",\n')
        f.write('           "extraArgs": {\n')
        f.write('                 "-vm": "metric",\n')
        f.write('                  "-vr": "7"}}]}}')
    


class TestMethods(unittest.TestCase):
    def test_buildCmd(self):
        data = [{ "infile": "foo.graph", "outfile": "bar.graph", "mode": "VGA", "extraArgs": { "-vm": "visibility", "-vg": "", "-vr": "5"}}]
        cmds = config.buildCmd( data )
        self.assertEqual(cmds[0].infile, "foo.graph")
        self.assertEqual(cmds[0].outfile, "bar.graph")
        self.assertEqual(cmds[0].mode, "VGA")
        self.assertEqual(len(cmds[0].extraArgs),3)


    def test_configClass(self):
        with DisposableFile("test.config") as testfile:
            writeConfig(testfile.filename(), "../foo/bar")
            conf = config.RegressionConfig(testfile.filename())
            self.assertEqual(len(conf.testcases), 1)
            self.assertEqual(conf.rundir, "../foo/bar")
            self.assertTrue("test1" in conf.testcases)


class TestRealConfig(unittest.TestCase):
    def test_realConfig(self):
        configFile = os.path.join("..", RegressionTestRunner.defaultConfigFile)
        self.assertNotEqual( configFile, "" )
        conf = config.RegressionConfig(configFile)
        self.assertFalse(conf.performanceRegression.enabled)

if __name__ == "__main__":
    unittest.main()
