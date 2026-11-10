"""Bounded process tests; temporary configuration and logs stay outside the repo."""

from __future__ import annotations

import argparse
import copy
import json
import signal
import subprocess
import tempfile
import time
from pathlib import Path


def require(condition: bool, message: str) -> None:
    """Keep test checks active independently of Python optimization flags."""
    if not condition:
        raise AssertionError(message)


def command(args: argparse.Namespace, config: Path, *, probe: bool = False) -> list[str]:
    """Use the same argument order as the project's run target."""
    executable = args.probe if probe else args.executable
    return [str(executable), str(config), "zgent", f"--hpx:config={args.hpx_config}"]


def run_process(arguments: list[str], directory: Path) -> subprocess.CompletedProcess[str]:
    """Collect a short-lived child; timeout always terminates and reaps it."""
    return subprocess.run(
        arguments, cwd=directory, capture_output=True, text=True, timeout=15, check=False
    )


def output(result: subprocess.CompletedProcess[str]) -> str:
    return result.stdout + result.stderr


def check_shutdown(text: str) -> None:
    """NNG must be finalized before reporting that both runtimes stopped."""
    nng_fini = text.find("[nng:runtime:fini]")
    stopped = text.find("[zgent:run] runtimes stopped")
    require(nng_fini >= 0 and stopped > nng_fini, text)
    hpx_returned = text.find("[hpx:run] returned")
    if hpx_returned >= 0:
        require(hpx_returned < nng_fini, text)


def check_signal(args: argparse.Namespace, config: Path, directory: Path, sig: int) -> None:
    """Wait for readiness, prove persistence, signal only our child, and reap it."""
    capture = directory / "console.log"
    with capture.open("w") as stream:
        process = subprocess.Popen(
            command(args, config), cwd=directory, stdout=stream, stderr=stream
        )
        try:
            deadline = time.monotonic() + 12
            while "[zgent:run] ready, hpx_workers=3" not in capture.read_text():
                require(process.poll() is None, capture.read_text())
                require(time.monotonic() < deadline, capture.read_text())
                time.sleep(0.05)
            time.sleep(0.3)
            require(process.poll() is None, "service exited without a stop request")
            process.send_signal(sig)
            status = process.wait(timeout=8)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
    text = capture.read_text()
    require(status == 0, text)
    require(text.find("[zgent:stop]") < text.find("[zgent:wait_stop]"), text)
    require("[zgent:stop]" in text and "[zgent:wait_stop]" in text, text)
    require(text.find("[hpx:run] returned") > text.find("[zgent:wait_stop]"), text)
    check_shutdown(text)
    # Every supported JSON setting must reach the NNG owner unchanged.
    for name, value in (
        ("num_task_threads", 1),
        ("max_task_threads", 2),
        ("num_expire_threads", 1),
        ("max_expire_threads", 2),
        ("num_poller_threads", 2),
        ("max_poller_threads", 2),
        ("num_resolver_threads", 1),
    ):
        require(f"{name}[{value}]" in text, text)


def check_configuration(
    args: argparse.Namespace, template: dict, config: Path, directory: Path
) -> None:
    """Exercise defaults, numeric bounds, types, and service selection."""
    def check(runtime: object, *, valid: bool, missing: bool = False) -> None:
        document = copy.deepcopy(template)
        if missing:
            document["zgent"].pop("nng_runtime")
        else:
            document["zgent"]["nng_runtime"] = runtime
        config.write_text(json.dumps(document))
        result = run_process(command(args, config, probe=True) + ["configure"], directory)
        require((result.returncode == 0) == valid, output(result))
        require("num_task_threads[" not in output(result), output(result))

    check({}, valid=True)
    check({}, valid=True, missing=True)
    check(None, valid=False)
    check([], valid=False)
    for key in template["zgent"]["nng_runtime"]:
        for bad in (True, "2", 1.5, None, 32768, -2, 2**63):
            check({key: bad}, valid=False)
        check({key: 0}, valid=True)
        check({key: 32767}, valid=True)
        check({key: -1}, valid=key.endswith("_max"))
    config.write_text(json.dumps({"other": template["zgent"]}))
    result = run_process(command(args, config, probe=True) + ["configure"], directory)
    require(result.returncode != 0, output(result))
    require("invalid service configuration" in output(result), output(result))
    result = run_process([str(args.executable)], directory)
    require(result.returncode != 0, output(result))


def check_startup_failure(args: argparse.Namespace, config: Path, directory: Path) -> None:
    """Failure before the HPX callback must still release the live NNG owner."""
    for option in (
        "--hpx:threads=invalid",
        f"--hpx:config={directory / 'missing.ini'}",
        f"--hpx:config={directory}",
        "--hpx:config=",
    ):
        arguments = command(args, config)
        if option.startswith("--hpx:config="):
            arguments[-1] = option
        else:
            arguments.append(option)
        result = run_process(arguments, directory)
        text = output(result)
        require(result.returncode != 0, text)
        require("[zgent:run] ready" not in text, text)
        check_shutdown(text)


def check_lifecycle(args: argparse.Namespace, config: Path, directory: Path) -> None:
    """Use the real runtimes to verify cleanup continuation and first error."""
    error_mask = 0x7F000000
    cases = {
        "success": 0,
        "single_run": 0,
        "loop_exception": error_mask | 1,
        "loop_and_cleanup_errors": error_mask | 1,
        "stop_exception": error_mask | 1,
        "wait_exception": error_mask | 1,
        "stop_error": error_mask | 9,
        "wait_error": error_mask | 8,
        "both_errors": error_mask | 9,
    }
    for scenario, expected in cases.items():
        result = run_process(command(args, config, probe=True) + [scenario], directory)
        text = output(result)
        require(result.returncode == expected % 256, text)
        require(f"probe:result={expected}" in text, text)
        require(text.count("probe:stop\n") == 1, text)
        require(text.count("probe:wait_stop\n") == 1, text)
        require(text.index("probe:stop\n") < text.index("probe:wait_stop\n"), text)
        check_shutdown(text)


def main() -> None:
    """Run one CTest scenario without credentials or external services."""
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "probe", "config", "hpx-config"):
        parser.add_argument(f"--{name}", type=Path, required=True)
    parser.add_argument("--scenario", required=True)
    args = parser.parse_args()
    template = json.loads(args.config.read_text())
    with tempfile.TemporaryDirectory(prefix="zgent-runtime-") as temporary:
        directory = Path(temporary)
        template["zgent"]["spd"].update(
            {
                "async": args.scenario in {"sigint", "sigterm"},
                "name": str(directory / "zgent.log"),
            }
        )
        config = directory / "zgent_conf.json"
        config.write_text(json.dumps(template))
        if args.scenario == "sigint":
            check_signal(args, config, directory, signal.SIGINT)
        elif args.scenario == "sigterm":
            check_signal(args, config, directory, signal.SIGTERM)
        elif args.scenario == "configuration":
            check_configuration(args, template, config, directory)
        elif args.scenario == "startup_failure":
            check_startup_failure(args, config, directory)
        elif args.scenario == "lifecycle":
            check_lifecycle(args, config, directory)
        else:
            raise ValueError(f"unknown scenario: {args.scenario}")


if __name__ == "__main__":
    main()
