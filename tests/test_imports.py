"""Guards the import mechanism every other test module depends on.

The repository-root conftest.py looks empty and deletable. It is not: its mere
presence is what makes pytest prepend the repository root to sys.path, which is
what lets these tests import dis6801, build and tools.report. Verified by
experiment — removing it makes `import dis6801` fail.
"""

import pathlib
import sys


def test_repository_root_is_on_syspath():
    root = str(pathlib.Path(__file__).resolve().parent.parent)
    assert root in sys.path, (
        "Repository root missing from sys.path. The root conftest.py has probably "
        "been deleted — it must exist for dis6801/build/tools imports to resolve."
    )


def test_dis6801_package_is_importable():
    import dis6801

    assert dis6801.__doc__


def test_root_conftest_exists():
    root = pathlib.Path(__file__).resolve().parent.parent
    assert (root / "conftest.py").is_file(), (
        "conftest.py at the repository root is load-bearing for sys.path; do not delete it."
    )
