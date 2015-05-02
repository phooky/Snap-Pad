import unittest
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
            if self.sp.readline().strip() == '---END DIAGNOSTICS---':
                break

    def testRNG(self):
        self.sp.write('#\n')
        v = self.sp.read(64)
        self.assertEqual(len(v),64)
        self.assertEqual(self.sp.read(1),'')

