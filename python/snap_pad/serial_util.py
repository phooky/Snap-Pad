import serial
import serial.tools.list_ports
import sys
import re
from snap_pad_vid_pid import vendor_id, product_id

linux_port_re = re.compile('VID:PID={0:04x}:{1:04x} SNR=([0-9A-Fa-f]+)'.format(vendor_id,product_id))

def make_candidate_linux(c):
	'Turn a comport entry into a (portname,serial#) tuple'
	m = linux_port_re.search(c[2])
	if m:
		return (c[0],m.group(1))
	else:
		return None


def find_snap_pads():
	if sys.platform.startswith('linux'):
		return filter(None,map(make_candidate_linux,serial.tools.list_ports.comports()))
	elif sys.platform.startswith('darwin'):
		pass
	elif sys.platform.startswith('win32'):
		pass
	elif syst.platform.startswith('cygwin'):
		pass


