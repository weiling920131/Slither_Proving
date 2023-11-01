import os
import re
import sys
import platform
import subprocess
from pathlib import Path

from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

import torch.utils


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = Path(sourcedir).resolve()


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('CMake must be installed to build the following extensions: ' +
                               ', '.join(e.name for e in self.extensions))

        if platform.system() == 'Windows':
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError('CMake >= 3.1.0 is required on Windows')

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = f'{Path(self.get_ext_fullpath(ext.name)).parent.resolve()}{os.sep}'

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == 'Windows':
            cmake_args += [f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}']
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = f'{env.get("CXXFLAGS", "")} -DVERSION_INFO=\\"{self.distribution.get_version()}\\"'

        cmake_prefix_path = [torch.utils.cmake_prefix_path]
        if 'CONDA_PREFIX' in env:
            cmake_prefix_path += [env['CONDA_PREFIX']]
        cmake_args += ['-DCMAKE_PREFIX_PATH=' + ';'.join(cmake_prefix_path)]

        Path(self.build_temp).mkdir(parents=True, exist_ok=True)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)


class CMakeFormatTarget(CMakeBuild):
    '''CMake format target'''
    def build_extension(self, ext):
        subprocess.check_call(
            ['cmake', '--build', '.', '--target', 'clangformat'],
            cwd=self.build_temp)


setup(
    name='clap',
    version='0.0.1',
    author='Yuan-Hao Chen',
    author_email='yhchen0906@gmail.com',
    description='CGI Lab AlphaZero Platform',
    long_description=Path('README.md').read_text(),
    long_description_content_type='text/markdown',
    url='https://github.com/yhchen0906/clap',
    packages=find_packages(),
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
    ext_modules=[CMakeExtension('clap')],
    cmdclass={
        'build_ext': CMakeBuild,
        'format': CMakeFormatTarget,
    },
    zip_safe=False,
)
