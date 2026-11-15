"""Owner-side gameplay chord validation independent of MCM and CEF."""
from pathlib import Path
import subprocess
import tempfile
import shutil
import unittest
ROOT=Path(__file__).resolve().parents[2]
class GameplayKeyPolicy(unittest.TestCase):
    def test_native_keybind_conversion_and_rejection(self):
        cxx=shutil.which('g++') or shutil.which('clang++')
        self.assertIsNotNone(cxx)
        source=r"""
#include "GameplayKeybindPolicy.h"
#include <cassert>
#include <utility>
using namespace k2040::gameplay_keys;
int main() {
 assert(Valid("Shift+K")&&Valid("Ctrl+Shift+K")&&Valid("Unbound"));
 assert(Valid("Mouse4")&&Valid("F12")&&Valid("Alt+F1"));
 for(auto invalid:{"Ctrl+Shift+B","Ctrl+Ctrl+K","Ctrl++K","K+Alt","",
                    "Mouse6","F13","Ctrl+Shift","Shift+Shift+K","K+"})
  assert(!Valid(invalid));
 assert(ToKeycode("Ctrl+Shift+K")==std::make_pair(75u,3u));
 assert(ToKeycode("Shift+F12")==std::make_pair(123u,1u));
 assert(ToKeycode("Mouse4")==std::make_pair(259u,0u));
 assert(FromKeycode(75,3)=="Ctrl+Shift+K");
 assert(FromKeycode(259,0)=="Mouse4");
 assert(FromKeycode(66,3).empty());
 assert(FromKeycode(222,0).empty());
}
"""
        with tempfile.TemporaryDirectory(prefix='gameplay-binding-',
                                         dir=Path.home()/'Downloads/K2040-Quick-Attach-Menu') as t:
            test=Path(t)/'test.cpp'
            output=Path(t)/'test'
            test.write_text(source)
            built=subprocess.run([cxx,'-std=c++23','-Wall','-Wextra','-Werror',
                                 '-I',str(ROOT/'include'),str(test),'-o',str(output)],
                                 capture_output=True,text=True,timeout=40)
            self.assertEqual(built.returncode,0,built.stderr[-1500:])
            run=subprocess.run([str(output)],capture_output=True,text=True,timeout=8)
            self.assertEqual(run.returncode,0,run.stderr[-800:])
if __name__=='__main__':unittest.main()
