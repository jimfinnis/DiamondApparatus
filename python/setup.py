from distutils.core import setup, Extension

module1 = Extension('diamondapparatus',
                    libraries=['diamondapparatus','pthread','stdc++'],
#                    extra_compile_args=['-g'],
                    sources = ['diamondpython.c'])

setup (name = 'DiamondApparatus',
       version = '1.0',
       description = 'Python linkage for DiamondApparatus',
       ext_modules = [module1])
