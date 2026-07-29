#!/usr/bin/env python3
import argparse
import importlib.util
import inspect
import os, re
import pathlib, tempfile
import sys
from typing import Any
import shutil

# Official Catalyst debugging API
from catalyst.debug import get_compilation_stage


def load_module_from_path(path: str):
    path = os.path.abspath(path)
    module_name = pathlib.Path(path).stem

    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load module from: {path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def resolve_function(module: Any, func_name: str):
    if not hasattr(module, func_name):
        raise RuntimeError(f"Function '{func_name}' not found in module '{module.__name__}'")
    return getattr(module, func_name)


def best_effort_compile(fn: Any):
    """
    Force compilation by calling the qjit-wrapped function once.

    Because argument requirements differ across user programs, this helper
    supports two modes:

    1. If the target function has zero required parameters, call it directly.
    2. Otherwise, require the user script to expose a helper function:
         __catalyst_compile_args__<function_name>()
       which returns either:
         - tuple(args)
         - (args, kwargs)

    Example in user's Python file:
        def __catalyst_compile_args__circuit():
            return ((0.5,), {})
    """
    sig = inspect.signature(fn)
    required_params = [
        p for p in sig.parameters.values()
        if p.default is inspect._empty
        and p.kind in (
            inspect.Parameter.POSITIONAL_ONLY,
            inspect.Parameter.POSITIONAL_OR_KEYWORD,
        )
    ]

    if len(required_params) == 0:
        fn()
        return

    owner_module = inspect.getmodule(fn)
    helper_name = f"__catalyst_compile_args__{fn.__name__}"

    if owner_module is None or not hasattr(owner_module, helper_name):
        req_names = ", ".join(p.name for p in required_params)
        raise RuntimeError(
            f"Function '{fn.__name__}' requires arguments ({req_names}). "
            f"Please define a helper named '{helper_name}' in the same Python file.\n"
            f"Example:\n"
            f"    def {helper_name}():\n"
            f"        return ((0.5,), {{}})\n"
        )

    helper = getattr(owner_module, helper_name)
    helper_result = helper()

    if isinstance(helper_result, tuple) and len(helper_result) == 2 and isinstance(helper_result[1], dict):
        args, kwargs = helper_result
    else:
        args, kwargs = helper_result, {}

    if not isinstance(args, tuple):
        raise RuntimeError(f"{helper_name} must return tuple(args) or (args, kwargs)")

    fn(*args, **kwargs)


def extract_ir(fn: Any, stage: str) -> str:
    """
    stage can be:
      - mlir
      - mlir_opt
      - qir
      - any valid Catalyst compilation stage name, e.g.
        QuantumCompilationStage, HLOLoweringStage, BufferizationStage, ...
    """
    # First try direct QJIT attributes, which Catalyst documents as available
    # when keep_intermediate is enabled.
    if stage in ("mlir", "mlir_opt", "qir", "jaxpr"):
        if hasattr(fn, stage):
            value = getattr(fn, stage)
            return str(value)
        raise RuntimeError(
            f"Requested '{stage}', but function does not expose that attribute. "
            "Ensure @qjit(..., keep_intermediate=True) is enabled."
        )

    # Otherwise interpret it as a pipeline stage name
    return str(get_compilation_stage(fn, stage))


def write_ir(ir_text: str, out_dir: str, func_name: str, stage: str) -> str:

    out_path = os.path.join(out_dir, f"{func_name}-{stage}.mlir")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(ir_text)
        if not ir_text.endswith("\n"):
            f.write("\n")
    return out_path


def clean(func_name:str):
    pattern = re.compile(rf"^{re.escape(func_name)}(?:_\d+)?$")
    base_dir = pathlib.Path(".")
    matches = [
        p for p in base_dir.iterdir()
        if p.is_dir() and pattern.match(p.name)
    ]

    names = [p.name for p in matches]
    print(names)
    for d in names:
        shutil.rmtree(d)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="Path to the Python source file")
    parser.add_argument("--function", required=True, help="Name of the qjit-decorated function")
    parser.add_argument(
        "--stage",
        default="mlir",
        help=(
            "IR selector. Examples: mlir, mlir_opt, qir, "
            "QuantumCompilationStage, HLOLoweringStage, BufferizationStage"
        ),
    )
    parser.add_argument("--out-dir", required=False, help="Directory to write extracted MLIR into")
    args = parser.parse_args()

    input_path = os.path.abspath(args.input)

    with tempfile.TemporaryDirectory(prefix="catalyst-ir-") as tmpdir:
        old_cwd = os.getcwd()
        try:
            os.chdir(tmpdir)

            module = load_module_from_path(input_path)
            fn = resolve_function(module, args.function)

            best_effort_compile(fn)

            ir_text = extract_ir(fn, args.stage)

        finally:
            os.chdir(old_cwd)

    if args.out_dir is not None:
        os.makedirs(args.out_dir, exist_ok=True)
        write_ir(ir_text, args.out_dir, args.function, args.stage)
    else:
        sys.stdout.write(ir_text)
        if not ir_text.endswith("\n"):
            sys.stdout.write("\n")


if __name__ == "__main__":
    main()
