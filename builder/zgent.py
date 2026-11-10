from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from zeta_forge.build_cli import Product, Project, Request
from zeta_forge.cmake_builder import CMakeProjectBuilder, CommonBuildArgs, cmake_bool
from zeta_forge.cmake_engine import CMAKE_ACTIONS, CMakeEngine
from zeta_forge.cmake_run import CMakeRunTarget
from zeta_forge.config import load_repo_config


@dataclass(frozen=True)
class ZgentBuildArgs(CommonBuildArgs):
    build_tests: bool


class ZgentBuilder(CMakeProjectBuilder):
    source_watch_patterns = (
        "CMakeLists.txt",
        "*.cmake",
        "*.cmake.in",
        "VERSION",
    )
    source_prune_dirs = ("build",)
    uses_conan = False

    @property
    def project_name(self) -> str:
        return "zgent"

    @property
    def typed_args(self) -> ZgentBuildArgs:
        return self.args  # type: ignore[return-value]

    @property
    def source_dir(self) -> Path:
        return self.repo_config.source_dir("ZETA_GENT_SRC_DIR")

    @property
    def cmake_util_dir(self) -> Path:
        return self.repo_config.forge_root / "cmake_util"

    @property
    def zeta_deps_cmake_dir(self) -> Path:
        return (
            self.repo_config.install_prefix / "lib" / "cmake" / "zeta_deps" / self.args.build_type
        )

    @property
    def missing_source_hint(self) -> str:
        return "Set ZETA_GENT_SRC_DIR to a local checkout or run from the zgent checkout with ./zbuild.py"

    def validate(self) -> None:
        super().validate()
        if not (self.cmake_util_dir / "common.cmake").is_file():
            raise RuntimeError(
                f"zeta_forge cmake_util not found: {self.cmake_util_dir}\n"
                "Ensure $ZETAX_ROOT/zeta_forge/cmake_util exists before building zgent."
            )
        if not self.zeta_deps_cmake_dir.is_dir():
            raise RuntimeError(
                f"ZetaX dependency package configs not found: {self.zeta_deps_cmake_dir}\n"
                "Install the shared dependency environment first with: "
                "$ZETAX_ROOT/zeta_forge/zbuild.py install deps --profile "
                f"{self.args.build_type.lower()}"
            )

    def configure_dependencies(self) -> list[Path]:
        return [
            self.script_path,
            Path(__file__),
            self.source_dir / "VERSION",
        ]

    def conan_install_command(self) -> list[object]:
        raise RuntimeError("zgent does not use Conan")

    def configure_command(self) -> list[object]:
        cmake_prefix_paths = [str(self.zeta_deps_cmake_dir), str(self.repo_config.install_prefix)]
        return [
            "cmake",
            "-S",
            self.source_dir,
            "-B",
            self.build_dir,
            "-G",
            "Ninja",
            "-Wno-dev",
            f"-DCMAKE_BUILD_TYPE={self.args.build_type}",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            f"-DCMAKE_PREFIX_PATH={';'.join(cmake_prefix_paths)}",
            f"-DCMAKE_INSTALL_PREFIX={self.repo_config.install_prefix}",
            f"-DCMAKE_CXX_STANDARD={self.repo_config.cxx_standard}",
            f"-DZETA_CMAKE_UTIL_DIR={self.cmake_util_dir}",
            f"-DZETA_DEPS_CMAKE_DIR={self.zeta_deps_cmake_dir}",
            f"-DZGENT_BUILD_TESTS={cmake_bool(self.typed_args.build_tests)}",
        ]


def project(script_path: Path) -> Project:
    root = script_path.resolve().parent
    config = load_repo_config(script_path, project_source_defaults={"ZETA_GENT_SRC_DIR": root})

    def factory(request: Request, _selected: tuple[str, ...]) -> ZgentBuilder:
        args = ZgentBuildArgs(
            request.cmake_profile,
            build_tests=request.action == "test",
        )
        return ZgentBuilder(script_path=script_path, repo_config=config, args=args)

    def run_environment(_target_name: str, _source_dir: Path) -> dict[str, str]:
        # zpp's async logger creates its rotating file before opening it.
        environment = dict(config.env)
        log_root = Path(environment["ZETAX_ROOT"]) / "logs" / "zgent"
        log_root.mkdir(parents=True, exist_ok=True)
        return environment

    engine = CMakeEngine(
        root,
        factory,
        ("zgent",),
        test_targets=lambda _: ("zgent", "zgent_lifecycle_probe"),
        run_environment=run_environment,
        run_targets={"zgent": CMakeRunTarget("zgent")},
    )
    products = (
        Product("zgent", "cmake", "zgent application server", (*CMAKE_ACTIONS, "run", "dev")),
    )
    return Project("zgent", products, {"cmake": engine}, ("zgent",), run_default="zgent")
