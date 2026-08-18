from tools.report import format_report, unreached_runs


def test_finds_runs_of_unreached_bytes():
    kind = ["code", "code", "unknown", "unknown", "unknown", "code", "unknown"]
    runs = unreached_runs(kind, base=0x8000, minimum=2)
    assert runs == [(0x8002, 3)]


def test_report_lists_unresolved_sites_and_largest_gaps():
    text = format_report(
        base=0x8000,
        kind=["code"] * 4 + ["unknown"] * 12,
        unresolved=[0x8001],
        bad_opcodes=[],
    )
    assert "$8001" in text
    assert "$8004" in text
    assert "12" in text
