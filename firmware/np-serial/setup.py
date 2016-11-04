#!/usr/bin/env python

try:
    import test
except:
    pass

try:
    from setuptools import setup, Command
    from setuptools.command.build import build
except ImportError:
    from distutils.core import setup, Command
    from distutils.command.build import build

classifiers = ['Development Status :: 4 - Beta',
               'Operating System :: MacOS :: MacOS X',
               'Operating System :: Microsoft :: Windows',
               'Operating System :: POSIX :: Linux',
               'License :: OSI Approved :: MIT License',
               'Intended Audience :: Developers',
               'Intended Audience :: End Users/Desktop',
               'Programming Language :: Python :: 2.6',
               'Programming Language :: Python :: 2.7',
               'Programming Language :: Python :: 3',
               'Topic :: Software Development',
               'Topic :: System :: Hardware']

setup(name		= 'nixiepipe',
        version		= '0.0.1',
        author		= 'John Whittington',
        author_email	= 'john@jbrengineering.co.uk',
        description	= 'Nixie Pipe serial driver',
        # long_description= open('README.txt').read() + open('CHANGELOG.txt').read(),
        license		= 'MIT',
        keywords	= 'Nixie Pipe serial communication',
        url		= 'http://www.jbrengineering.co.uk',
        classifiers	= classifiers,
        py_modules	= [],
        packages	= ['nixiepipe'],
        include_package_data = True,
        install_requires= ['pyserial'],
)
