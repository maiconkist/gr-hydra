import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from xml.dom import minidom


ROOT = Path(__file__).resolve().parents[1]


def load_script(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PythonCompatibilityTests(unittest.TestCase):
    def test_repository_python_sources_compile(self):
        for folder in ("app", "grc_blocks", "tests"):
            for path in (ROOT / folder).rglob("*.py"):
                if "build" in path.relative_to(ROOT).parts:
                    continue
                with self.subTest(script=str(path.relative_to(ROOT))):
                    compile(path.read_bytes(), str(path), "exec")

    def test_legacy_template_generation(self):
        folder = ROOT / "grc_blocks" / "python"
        with patch.object(sys, "path", [str(folder), *sys.path]):
            build_utils = load_script(folder / "build_utils.py")
        values = build_utils.standard_dict("hydra_filter", "ccf", "hydra")
        output = io.StringIO()
        build_utils.do_substitution(
            values,
            io.StringIO("@I_TYPE@ @NAME@(@TAP_TYPE@ gain);"),
            output,
        )
        self.assertEqual(output.getvalue(), "gr_complex hydra_filter(float gain);")

    def test_doxygen_generation_and_unicode_xml_round_trip(self):
        folder = ROOT / "grc_blocks" / "docs" / "doxygen"
        with patch.object(sys, "path", [str(folder), *sys.path]):
            from doxyxml.generated import compound, index

        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            index_file = directory / "index.xml"
            index_file.write_text(
                '<doxygenindex version="1.8.0">'
                '<compound kind="class" refid="classdemo">'
                '<name>Demo</name></compound></doxygenindex>',
                encoding="utf-8",
            )
            compound_file = directory / "classdemo.xml"
            compound_file.write_text(
                '<doxygen version="1.8.0">'
                '<compounddef kind="class" id="classdemo">'
                '<compoundname>Demo</compoundname>'
                '<briefdescription><para>Reads "caf\u00e9" samples.</para>'
                '</briefdescription>'
                '<detaileddescription><para>Detailed text.</para>'
                '</detaileddescription>'
                '<sectiondef kind="public-func">'
                '<memberdef kind="function" id="read"><name>read</name>'
                '<briefdescription><para>Read samples.</para></briefdescription>'
                '<detaileddescription><para>Returns samples.</para>'
                '</detaileddescription>'
                '<param><type>int</type><declname>count</declname></param>'
                '</memberdef></sectiondef></compounddef></doxygen>',
                encoding="utf-8",
            )
            output_file = directory / "swig_doc.i"
            result = subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(folder / "swig_doc.py"),
                    str(directory),
                    str(output_file),
                ],
                capture_output=True,
                text=True,
                timeout=30,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            documentation = output_file.read_text(encoding="utf-8")
            self.assertIn('Reads \\"caf?\\" samples.', documentation)
            self.assertIn('%feature("docstring") Demo::read', documentation)
            self.assertIn("Params: (count)", documentation)

            for parser, path, root_name in (
                (index, index_file, "doxygenindex"),
                (compound, compound_file, "doxygen"),
            ):
                with self.subTest(parser=root_name):
                    output = io.StringIO()
                    parser.parse(str(path)).export(output, 0, name_=root_name)
                    with minidom.parseString(output.getvalue()) as document:
                        self.assertEqual(document.documentElement.tagName, root_name)
                    if parser is compound:
                        self.assertIn("caf\u00e9", output.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=2)
