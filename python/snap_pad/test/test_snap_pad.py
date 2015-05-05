import unittest
import re
from test_snap_pad_mock import SnapPadHWMock, MAJOR, MINOR
from snap_pad import SnapPad

class SnapPadTest(unittest.TestCase):
    def setUp(self):
        self.mock = SnapPadHWMock()
        self.mock.diagnostics['testkey']='testval'
        self.sp = SnapPad(self.mock,'MOCK')

    def tearDown(self):
        self.mock = None
        self.sp = None

    def testVersion(self):
        self.assertEqual(self.sp.major, MAJOR)
        self.assertEqual(self.sp.minor, MINOR)
        self.assertEqual(self.sp.variant, 'M')

    def testDiagnostics(self):
        self.assertEqual(self.sp.diagnostics['testkey'],'testval')

    def doProvision(self,count):
        paras = self.sp.provision_paragraphs(count)        
        self.assertEqual(len(paras),count)
        for para in paras:
            self.assertEqual(para.used, False)
            self.assertEqual(len(para.bits),512)

    def doRetrieval(self, addrs):
        paras = self.sp.retrieve_paragraphs(addrs)
        self.assertEqual(len(paras),len(addrs))
        i=0
        for para in paras:
            self.assertEqual((para.block,para.page,para.para),addrs[i])
            self.assertEqual(para.used, False)
            self.assertEqual(len(para.bits),512)
            i = i+1

    def testRetrievalOne(self):
        self.doRetrieval([(1,2,3)])

    def testRetrievalFour(self):
        self.doRetrieval([(1,2,3),(2,3,2),(3,4,1),(4,5,0)])

    def testProvisionOne(self):
        self.doProvision(1)

    def testProvisionFour(self):
        self.doProvision(4)


