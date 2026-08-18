"""Root conftest.

This file is intentionally almost empty. Its *presence* is what makes pytest
prepend the repository root to sys.path, which is how tests import dis65xx,
build and tools.report. Deleting it breaks every test module.

Verified by experiment: with this file moved aside, `import dis65xx` fails and
the repository root is absent from sys.path. See tests/test_imports.py.
"""
