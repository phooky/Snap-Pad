import unittest
import re
from .test_snap_pad_mock import SnapPadHWMock, MAJOR, MINOR
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
        pages = self.sp.provision_pages(count)        
        self.assertEqual(len(pages),count)
        for page in pages:
            self.assertEqual(page.used, False)
            self.assertEqual(len(page.bits),2048)

    def doRetrieval(self, addrs):
        pages = self.sp.retrieve_pages(addrs)
        self.assertEqual(len(pages),len(addrs))
        i=0
        for page in pages:
            self.assertEqual(page.page,addrs[i])
            self.assertEqual(page.used, False)
            self.assertEqual(len(page.bits),2048)
            i = i+1

    def testRetrievalOne(self):
        self.doRetrieval([64])

    def testRetrievalFour(self):
        self.doRetrieval([64,65,66,67])

    def testProvisionOne(self):
        self.doProvision(1)

    def testProvisionFour(self):
        self.doProvision(4)

    def testRandom(self):
        r = self.sp.hwrng()
        self.assertEqual(len(r),64)

