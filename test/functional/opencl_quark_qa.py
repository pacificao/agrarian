#!/usr/bin/env python3
"""Offline correctness test for a pinned OpenCL Quark kernel. No mining or pool traffic."""

import argparse
import ctypes as C
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import sys
import tarfile

SOURCE_REVISION = "002021095feb64065c902863cf717af7f94854bd"
SOURCE_SHA256 = "53692928220f426930f334d684268e8ea92c078199d033fefff3fcb45afdb501"
KERNEL_FILES = {"quarkcoin.cl", "blake.cl", "bmw.cl", "groestl.cl", "jh.cl",
                "keccak.cl", "skein.cl"}
BATCH = 96
P, U, I, S, L = C.c_void_p, C.c_uint32, C.c_int32, C.c_size_t, C.c_uint64


def checked(status, operation):
    if status:
        raise RuntimeError(f"{operation}: OpenCL error {status}")


def source_from_archive(path, screen):
    if hashlib.sha256(path.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise ValueError("OpenCL source archive does not match the pinned SHA-256")
    with tarfile.open(path, "r:gz") as archive:
        files = {name: archive.extractfile(f"sgminer-{SOURCE_REVISION}/kernel/{name}").read().decode("utf-8")
                 for name in KERNEL_FILES}
    source = re.sub(r'^#include "([a-z]+\.cl)"$', lambda m: files[m[1]],
                    files["quarkcoin.cl"], flags=re.MULTILINE)
    if re.search(r'^\s*#\s*include\b', source, flags=re.MULTILINE):
        raise ValueError("Unexpected kernel include")
    # OpenCL C requires __local declarations at kernel scope. Keep the
    # first Groestl phase's scratch separate from the second phase's.
    tables = re.compile(r"#if !SPH_SMALL_FOOTPRINT_GROESTL\n  __local sph_u64 T0_C\[256\].*?#endif\n", re.DOTALL)
    declarations = list(tables.finditer(source))
    if len(declarations) != 2:
        raise ValueError("Unexpected Groestl table layout")
    first, second = declarations

    def first_phase(fragment):
        return re.sub(r"\b(T[0-7])_C\b", r"\1_STAGE1", fragment)

    source = (source[:first.start()] + first_phase(source[first.end():second.start()]) +
              source[second.start():])
    point = "  uint gid = get_global_id(0);"
    if source.count(point) != 1:
        raise ValueError("Unexpected Quark entry point")
    source = source.replace(point, first_phase(first.group()) +
                            "  uint lane = get_global_id(0);\n  uint gid = base_nonce + lane;")
    source = source.replace("#define SPH_SMALL_FOOTPRINT_GROESTL 0",
                            "#define SPH_SMALL_FOOTPRINT_GROESTL 1")

    # Unlike a pool worker, write an entire digest at one private output slot per
    # lane, with no share filter, random index or concurrent writes.
    entry = "volatile __global uint* output, const ulong target"
    finish = "bool result = (SWAP8(hash.h8[3]) <= target);\n  if (result)\n    output[output[0xFF]++] = SWAP4(gid);"
    if source.count(entry) != 1 or source.count(finish) != 1:
        raise ValueError("Unexpected pinned kernel structure")
    if screen:
        # Stage one of pool validation: a conservative high-word filter.
        # Flag equal high words so the host can compare the full 256 bits.
        args = "__global uchar* output, const uint base_nonce, const ulong target"
        body = "output[lane] = (SWAP8(hash.h8[3]) <= target);"
    else:
        args = "__global ulong* output, const uint base_nonce"
        body = "for (uint i = 0; i < 4; ++i)\n    output[4 * lane + i] = SWAP8(hash.h8[i]);"
    return source.replace(entry, args).replace(finish, body).encode("utf-8")


class OpenCL:
    def __init__(self):
        self.lib = C.WinDLL("OpenCL.dll", winmode=0x800) if sys.platform == "win32" else C.CDLL("libOpenCL.so.1")
        signature = {
            "clGetPlatformIDs": (I, U, P, P), "clGetDeviceIDs": (I, P, L, U, P, P),
            "clGetDeviceInfo": (I, P, U, S, P, P), "clCreateContext": (P, P, U, P, P, P, P),
            "clCreateCommandQueue": (P, P, P, L, P), "clCreateProgramWithSource": (P, P, U, P, P, P),
            "clBuildProgram": (I, P, U, P, C.c_char_p, P, P), "clGetProgramBuildInfo": (I, P, P, U, S, P, P),
            "clCreateKernel": (P, P, C.c_char_p, P), "clCreateBuffer": (P, P, L, S, P, P),
            "clGetKernelWorkGroupInfo": (I, P, P, U, S, P, P),
            "clSetKernelArg": (I, P, U, S, P), "clEnqueueNDRangeKernel": (I, P, P, U, P, P, P, U, P, P),
            "clEnqueueWriteBuffer": (I, P, P, U, S, S, P, U, P, P),
            "clEnqueueReadBuffer": (I, P, P, U, S, S, P, U, P, P),
        }
        for item in ("Context", "CommandQueue", "Program", "Kernel", "MemObject"):
            signature["clRelease" + item] = (I, P)
        for name, (ret, *args) in signature.items():
            fn = getattr(self.lib, name)
            fn.restype, fn.argtypes = ret, args
            setattr(self, name.removeprefix("cl"), fn)

    def devices(self, allow_software):
        platforms = U()
        err = self.GetPlatformIDs(0, None, C.byref(platforms))
        if err == -1001:
            return
        checked(err, "enumerate OpenCL platforms")
        platform_list = (P * platforms.value)()
        checked(self.GetPlatformIDs(platforms, platform_list, None), "get OpenCL platforms")
        kinds = 4 | (2 if allow_software else 0)  # GPU, optionally CPU for QA only
        for platform in platform_list:
            count = U()
            err = self.GetDeviceIDs(platform, kinds, 0, None, C.byref(count))
            if err == -1:
                continue
            checked(err, "enumerate devices")
            devices = (P * count.value)()
            checked(self.GetDeviceIDs(platform, kinds, count, devices, None), "get devices")
            yield from devices

    def info(self, device, prop):
        length = S()
        checked(self.GetDeviceInfo(device, prop, 0, None, C.byref(length)), "device info size")
        data = C.create_string_buffer(length.value)
        checked(self.GetDeviceInfo(device, prop, length, data, None), "device info data")
        return data.value.decode("utf-8", errors="replace")

    def hashes(self, device, source, vectors, work_group, targets):
        cleanup = []

        def create(operation, release, *args):
            errno = I()
            handle = operation(*args, C.byref(errno))
            checked(errno.value, operation.__name__)
            cleanup.append((release, handle))
            return handle

        try:
            devices = (P * 1)(device)
            context = create(self.CreateContext, self.ReleaseContext, None, 1, devices, None, None)
            queue = create(self.CreateCommandQueue, self.ReleaseCommandQueue, context, device, 0)
            program = create(self.CreateProgramWithSource, self.ReleaseProgram, context, 1,
                             (C.c_char_p * 1)(source), None)
            options = f"-cl-std=CL1.2 -DWORKSIZE={work_group}".encode("ascii")
            err = self.BuildProgram(program, 1, devices, options, None, None)
            if err:
                length = S()
                self.GetProgramBuildInfo(program, device, 0x1183, 0, None, C.byref(length))
                log = C.create_string_buffer(length.value or 1)
                self.GetProgramBuildInfo(program, device, 0x1183, length, log, None)
                raise RuntimeError(log.value.decode("utf-8", errors="replace"))
            kernel = create(self.CreateKernel, self.ReleaseKernel, program, b"search")
            for prop, kind, label in ((0x11B0, S, "max-group"), (0x11B2, L, "local-bytes"),
                                       (0x11B4, L, "private-bytes")):
                value = kind()
                checked(self.GetKernelWorkGroupInfo(kernel, device, prop, C.sizeof(value),
                                                   C.byref(value), None), label)
                print(f"  {label}={value.value}", flush=True)
            inp = create(self.CreateBuffer, self.ReleaseMemObject, context, 4, 80, None)
            out_size = BATCH if targets is not None else BATCH * 32
            out = create(self.CreateBuffer, self.ReleaseMemObject, context, 2, out_size, None)
            for index, val in enumerate((inp, out)):
                checked(self.SetKernelArg(kernel, index, C.sizeof(P), C.byref(P(val))), "set kernel arg")
            for index, (header, offset) in enumerate(vectors):
                checked(self.SetKernelArg(kernel, 2, C.sizeof(U), C.byref(U(offset))), "set nonce base")
                if targets is not None:
                    checked(self.SetKernelArg(kernel, 3, C.sizeof(L), C.byref(L(targets[index]))), "set upper target")
                checked(self.EnqueueWriteBuffer(queue, inp, 1, 0, 80, C.create_string_buffer(header), 0, None, None),
                        "write header")
                checked(self.EnqueueNDRangeKernel(queue, kernel, 1, None, C.byref(S(BATCH)),
                                                  C.byref(S(work_group)), 0, None, None), f"launch at nonce {offset}")
                digest = C.create_string_buffer(out_size)
                checked(self.EnqueueReadBuffer(queue, out, 1, 0, len(digest), digest, 0, None, None),
                        f"read digests from nonce {offset}")
                yield digest.raw
        finally:
            for release, handle in reversed(cleanup):
                release(handle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_archive", type=Path)
    reference = parser.add_mutually_exclusive_group(required=True)
    reference.add_argument("--core-reference", type=Path)
    reference.add_argument("--reference-digests", type=Path, help="digests generated on another host by the Core reference")
    parser.add_argument("--export-reference", type=Path, help="write Core digests and exit without opening an OpenCL device")
    parser.add_argument("--allow-software", action="store_true", help="also check CPU OpenCL; does not qualify a GPU")
    parser.add_argument("--work-group", type=int, choices=(8, 16, 32), default=8)
    parser.add_argument("--screen", action="store_true", help="also test the GPU's conservative share candidate filter")
    args = parser.parse_args()
    vectors = [(bytes(80), 0), (bytes(range(80)), 0x123456), (bytes(reversed(range(80))), 2**32 - BATCH)]
    if args.core_reference:
        expected = subprocess.run([str(args.core_reference.resolve(strict=True))], check=True, capture_output=True,
                                  input=b"".join(h[:76] + struct.pack("<I", n)
                                                 for h, start in vectors for n in range(start, start + BATCH))).stdout
    else:
        expected = args.reference_digests.read_bytes()
    if len(expected) != len(vectors) * BATCH * 32:
        parser.error("reference digest file does not match the test batch size")
    if args.export_reference:
        if not args.core_reference:
            parser.error("--export-reference requires --core-reference")
        with args.export_reference.open("xb") as out:
            out.write(expected)
        return
    source = source_from_archive(args.source_archive, False)
    api = OpenCL()
    devices = list(api.devices(args.allow_software))
    if not devices:
        parser.exit(2, "No eligible OpenCL devices found. Check the vendor's graphics driver.\n")
    for device in devices:
        label = " | ".join(api.info(device, prop) for prop in (0x102C, 0x102B, 0x102D))
        print(f"Checking {label}", flush=True)
        actual = b"".join(api.hashes(device, source, vectors, args.work_group, None))
        if len(actual) != len(expected) or actual != expected:
            for i in range(0, min(len(actual), len(expected)), 32):
                if actual[i:i+32] != expected[i:i+32]:
                    raise AssertionError(f"Vector {i//32}: OpenCL {actual[i:i+32].hex()} != Core {expected[i:i+32].hex()}")
            raise AssertionError("OpenCL returned an unexpected number of digests")
        print(f"PASS: {len(expected)//32} complete Quark digests matched Agrarian Core.", flush=True)
        if args.screen:
            first_equal = int.from_bytes(expected[24:32], "little")
            targets = (first_equal, 2**62, 2**64 - 1)
            flags = b"".join(api.hashes(device, source_from_archive(args.source_archive, True),
                                        vectors, args.work_group, targets))
            expect_flags = bytes(int.from_bytes(expected[index+24:index+32], "little") <=
                                 targets[index // (32 * BATCH)] for index in range(0, len(expected), 32))
            if flags != expect_flags:
                raise AssertionError("GPU share candidate filter differs from Core high-word comparison")
            print(f"PASS: all {len(flags)} share candidate decisions matched Core.", flush=True)


if __name__ == "__main__":
    main()
