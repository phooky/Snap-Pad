import unittest
import re
from test_snap_pad_mock import SnapPadHWMock

class MockTest(unittest.TestCase):
    def setUp(self):
        self.sp = SnapPadHWMock()

    def tearDown(self):
        self.sp = None

    def testVersion(self):
        self.sp.write("V\n")
        v = self.sp.readline().strip()
        self.assertEqual(v,'{0}.{1}M'.format(self.sp.major,self.sp.minor))
        self.assertEqual(self.sp.readline(),'')

    def testDiagnostics(self):
        self.sp.write('D\n')
        self.assertEqual(self.sp.readline().strip(),'---BEGIN DIAGNOSTICS---')
        while True:
            v = self.sp.readline().strip()
            self.assertNotEqual(v,'')
            if v == '---END DIAGNOSTICS---':
                break

    def testRNG(self):
        self.sp.write('#\n')
        v = self.sp.read(64)
        self.assertEqual(len(v),64)
        self.assertEqual(self.sp.read(1),'')

    def assertErrorMsg(self,msg):
        r = self.sp.readline().strip()
        prefix = "ERROR: "
        self.assertEqual(r[:len(prefix)],prefix)
        self.assertEqual(r[len(prefix):],msg)

    def parsePara(self):
        first = self.sp.readline().strip()
        m = re.match('---BEGIN PARA ([0-9]+),([0-9]+),([0-9]+)---',first)
        self.assertIsNotNone(m)
        while True:
            l = self.sp.readline().strip()
            self.assertNotEqual(l,'')
            if re.match('---END PARA---',l):
                break
            self.assertIsNotNone(re.match('[A-Za-z0-9+/]+=?=?$',l))
        return map(int,m.groups()) 

    def doProvisionTest(self,count):
        self.sp.write('P{0}\n'.format(count))
        if count < 1 or count > 4:
            # check for error msg
            self.assertErrorMsg("bad count")
        else:
            # check for random data
            for _ in range(count):
                self.parsePara()

    def testProvision(self):
        for i in range(0,6):
            self.doProvisionTest(i)

    def doRetrievalTest(self,addresses):
        self.assertGreater(len(addresses),0)
        self.assertLessEqual(len(addresses),4)
        cmd = 'R' + ','.join([','.join(map(str,x)) for x in addresses]) + '\n'
        self.sp.write(cmd)
        for a in addresses:
            r = self.parsePara()
            self.assertEqual(r,a)

    def testSingleRetrieval(self):
        self.doRetrievalTest([[1,2,3]])
    
    def testMultipleRetrieval(self):
        self.doRetrievalTest([[1,2,3],[2,3,4],[3,4,5],[4,5,6]])



