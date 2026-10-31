"""Build configuration checks, runnable without an installed KCore."""
import configparser
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('precision_dist', ROOT / 'KCore/Dist.py')
Dist = importlib.util.module_from_spec(spec)
spec.loader.exec_module(Dist)


class PrecisionBuildTests(unittest.TestCase):
    def setUp(self):
        self.environ = patch.dict(os.environ, {}, clear=True)
        self.environ.start()
        self.addCleanup(self.environ.stop)
        Dist.CONFIGDICT = {'Cppcompiler': 'gcc', 'f77compiler': 'gfortran', 'useOMP': False}

    def test_default_and_suffix(self):
        self.assertEqual(Dist.getPrecision(), 'double')
        os.environ['ELSAPROD'] = 'ubuntu_r4_i8_DBG'
        self.assertEqual(Dist.getPrecision(), 'single')

    def test_invalid_selection(self):
        for value in ('float16', '', 'fp64'):
            os.environ['CASSIOPEE_PRECISION'] = value
            with self.assertRaises(ValueError):
                Dist.getCArgs()

    def test_inconsistent_selection(self):
        os.environ.update(ELSAPROD='ubuntu_r4', CASSIOPEE_PRECISION='double')
        with self.assertRaises(ValueError):
            Dist.getPrecision()

    def test_fortran_and_cpp_agree(self):
        for mode, size in (('single', 4), ('double', 8)):
            with self.subTest(precision=mode):
                os.environ['CASSIOPEE_PRECISION'] = mode
                self.assertIn('REAL*'+str(size), Dist.getPP())
                self.assertEqual('-DE_SINGLEREAL' in Dist.getCArgs(), size == 4)
                self.assertEqual('-fdefault-real-8' in Dist.getForArgs(), size == 8)

    def test_extension_caches_are_separate(self):
        with tempfile.TemporaryDirectory() as tmp:
            previous = os.getcwd()
            try:
                os.chdir(tmp)
                paths = []
                for mode in ('single', 'double'):
                    os.environ['CASSIOPEE_PRECISION'] = mode
                    Dist.writeSetupCfg()
                    cfg = configparser.ConfigParser()
                    cfg.read('setup.cfg')
                    paths.append(cfg['build']['build_base'])
                    self.assertEqual(cfg['build_ext']['compiler'], 'unix')
                    self.assertTrue(cfg['build_ext'].getboolean('force'))
                self.assertNotEqual(*paths)
            finally:
                os.chdir(previous)


if __name__ == '__main__':
    unittest.main()
