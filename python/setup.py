''' Setuptools configuration file for Snap-Pad tools. '''

from setuptools import setup, find_packages
from os import path
from codecs import open

setup( 
	name = 'snap_pad',
	version = '1.0.0a1',
	description = 'Module for interacting with the Snap-Pad one time pad device',
	author= 'Adam Mayer',
	author_email = 'phooky@gmail.com',
	license = 'BSD',
	url = 'http://snap-pad.info/',
	classifiers = [
		'License :: OSI Approved :: BSD License',
		'Operating System :: POSIX :: Linux',
		'Programming Language :: Python :: 2',
		'Topic :: Security',
		'Topic :: Communications',
		'Development Status :: 3 - Alpha'
	],
	keywords='snap-pad otp',
	packages=find_packages(exclude=['snap_pad.test']),
	install_requires=[
		'pyserial>=2.6',
	],
	entry_points={
		'console_scripts' : [
			'snap-pad=snap_pad:main'
		]
	},
)