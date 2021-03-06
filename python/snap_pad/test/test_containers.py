import unittest
import re
from snap_pad import SnapPad, PAGESIZE
from snap_pad.containers import EncryptedMessage
import array
import random
import math

class EncryptedMessageTest(unittest.TestCase):
    def setUp(self):
        self.enc = EncryptedMessage()
        for i in range(1,5):
            self.enc.add_block(i,'*'*(2**i),chr(i)*32)

    def tearDown(self):
        pass

    def testEq(self):
        for i in range(1,5):
            alt = EncryptedMessage()
            for j in range(1,i+1):
                alt.add_block(j, '*'*(2**j),chr(j)*32)
            if i  == 4:
                self.assertEqual(alt,self.enc)
                self.assertEqual(self.enc,alt)
            else:
                self.assertNotEqual(alt,self.enc)
                self.assertNotEqual(self.enc,alt)

    def testAscii(self):
        a = EncryptedMessage.to_ascii(self.enc)
        e2 = EncryptedMessage.from_ascii(a)
        self.assertEqual(self.enc, e2)

    def testJson(self):
        a = EncryptedMessage.to_json(self.enc)
        e2 = EncryptedMessage.from_json(a)
        self.assertEqual(self.enc, e2)

    def testBinary(self):
        b = EncryptedMessage.to_binary(self.enc)
        e2 = EncryptedMessage.from_binary(b)
        self.assertEqual(e2, self.enc)

    def testMismatches(self):
        j = EncryptedMessage.to_json(self.enc)
        b = EncryptedMessage.to_binary(self.enc)
        a = EncryptedMessage.to_ascii(self.enc)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_json(b)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_json(a)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_binary(j)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_binary(a)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_ascii(j)
        with self.assertRaises(Exception) as context:
            EncryptedMessage.from_ascii(b)


    def testRound(self):
        a = EncryptedMessage.to_json(self.enc)
        e2 = EncryptedMessage.from_json(a)
        self.assertEqual(self.enc, e2)
        b = EncryptedMessage.to_binary(e2)
        e3 = EncryptedMessage.from_binary(b)
        self.assertEqual(e3, e2)
        c = EncryptedMessage.to_ascii(e2)
        e4 = EncryptedMessage.from_ascii(c)
        self.assertEqual(e4, self.enc)

    def makeTestMsg(self,len):
        return b''.join([chr(random.randint(0x20,0x5a)) for i in range(len)])

