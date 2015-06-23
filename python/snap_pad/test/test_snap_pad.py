import unittest
import re
from .test_snap_pad_mock import SnapPadHWMock, MAJOR, MINOR
from snap_pad import SnapPad, PAGESIZE, Plaintext
from snap_pad.snap_pad import Page
import array
import random
import math

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
            self.assertEqual(page.page_idx,addrs[i])
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

    def makeRandPage(self):
        pageidx = random.randint(64,2048*64)
        page = Page(pageidx)
        page.used = False
        page.bits = array.array('B',[random.randint(0,255) for i in range(2048)])
        return page

    def makeTestMsg(self,len):
        return b''.join([chr(random.randint(0x20,0x5a)) for i in range(len)])

    def testCrypt(self):
        page = self.makeRandPage()
        pt = self.makeTestMsg(1000)
        ct = page.crypt(pt)
        self.assertNotEqual(ct, pt)
        self.assertEqual(len(ct),len(pt))
        self.assertEqual(page.crypt(ct), pt)

    def testSign(self):
        page = self.makeRandPage()
        pt = self.makeTestMsg(1000)
        ct = page.crypt(pt)
        sig = page.sign(ct)
        self.assertEqual(len(sig), 32)
        self.assertTrue(page.verify(ct,sig))

    def testEncryptAndSign(self):
        def teasSize(self,msgsz):
            blocks = math.floor(float(msgsz-1)/PAGESIZE) + 1
            testmsg = self.makeTestMsg(msgsz)
            self.assertEqual( len(self.sp.encrypt_and_sign(testmsg).blocks), blocks)
            self.assertGreater(blocks,0)
            self.assertLessEqual(blocks,4)
        for testsz in [PAGESIZE-1,PAGESIZE,PAGESIZE+1,PAGESIZE*2+1,PAGESIZE*3+1,PAGESIZE*4]:
            teasSize(self,testsz)

    def testEncryptAndSignOversize(self):
        with self.assertRaises(AssertionError):
            self.sp.encrypt_and_sign(self.makeTestMsg((PAGESIZE*4)+1))

    def testDecryptAndVerify(self):
        def tdavSize(self,msgsz):
            blocks = math.floor(float(msgsz-1)/PAGESIZE) + 1
            testmsg = self.makeTestMsg(msgsz)
            enc = self.sp.encrypt_and_sign(testmsg)
            pt = self.sp.decrypt_and_verify(enc)
            self.assertEqual( len(pt.data), msgsz)
            self.assertTrue(pt.signed)
            self.assertTrue(pt.sig_good)
            self.assertEqual(pt.data,testmsg)
        tdavSize(self,100)
        for testsz in [PAGESIZE-1,PAGESIZE,PAGESIZE+1,PAGESIZE*2+1,PAGESIZE*3+1,PAGESIZE*4]:
            tdavSize(self,testsz)

    def testRandom(self):
        r = self.sp.hwrng()
        self.assertEqual(len(r),64)


    def testPadMsg(self):
        def tpm(self,msgsz):
            msg = self.makeTestMsg(msgsz)
            blocks = math.floor(float(msgsz-1)/PAGESIZE) + 1
            pt = Plaintext(msg)
            self.assertEqual(len(pt.data),msgsz)
            pt.pad_text(self.sp)
            self.assertEqual(len(pt.data),blocks*PAGESIZE)
            pt.unpad_text()
            self.assertEqual(len(pt.data),msgsz)
        for testsz in [PAGESIZE-1,PAGESIZE,PAGESIZE+1,PAGESIZE*2+1,PAGESIZE*3+1,PAGESIZE*4]:
            tpm(self,testsz)
