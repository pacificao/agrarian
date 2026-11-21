#!/usr/bin/env python3
"""Offline stake-bound sensitivity report from a verified canonical snapshot.

Use an immutable mainnet blk file and its independent replay
JSON. This measures output sizes, not stake participation or wallet ownership.
No addresses, keys, or transaction IDs are printed.
"""
import argparse
import io
import json
import struct
from collections import Counter
from decimal import Decimal
from pathlib import Path

from miningcore_staging import miningcore_quark
from test_hybrid_branch_model import decode_target
from testnet_hybrid import sha256d


def read(stream, size):
    data = stream.read(size)
    if len(data) != size:
        raise ValueError("incomplete block or transaction")
    return data


def compact(stream):
    tag = read(stream, 1)[0]
    return (int.from_bytes(read(stream, {253: 2, 254: 4, 255: 8}[tag]), "little")
            if tag >= 253 else tag)


def transaction(stream):
    start = stream.tell()
    read(stream, 4)
    inputs = []
    for _ in range(compact(stream)):
        h = read(stream, 32)
        n = int.from_bytes(read(stream, 4), "little")
        script = read(stream, compact(stream))
        if not any(h) and n == 0xffffffff and script[:1] in (b"\xc2", b"\xc3"):
            raise ValueError("accumulator spends require independent UTXO treatment")
        read(stream, 4)
        inputs.append((h, n))
    outputs = []
    for _ in range(compact(stream)):
        value = struct.unpack("<q", read(stream, 8))[0]
        outputs.append((value, read(stream, compact(stream))))
    read(stream, 4)
    return sha256d(stream.getbuffer()[start:stream.tell()]), inputs, outputs


def collect(snapshot, multihash, expected_tip):
    genesis_parent = bytes(32)
    blocks = {}
    with snapshot.open("rb") as stream:
        while True:
            envelope = stream.read(8)
            if not envelope:
                break
            if not any(envelope):
                if any(stream.read()):
                    raise ValueError("nonzero data after padding")
                break
            magic, size = struct.unpack("<4sI", envelope)
            if magic != bytes.fromhex("0b23a0b9") or size < 81:
                raise ValueError("invalid mainnet block envelope")
            payload = read(stream, size)
            block_hash = miningcore_quark(multihash, payload[:80])
            if blocks.setdefault(block_hash, payload) != payload:
                raise ValueError("ambiguous snapshot block payload")

    # Block files can contain side blocks or be out of order. Use only the
    # ancestors of the tip reported by the independent daemon replay.
    branch = []
    next_hash = bytes.fromhex(expected_tip)[::-1]
    while next_hash != genesis_parent:
        payload = blocks.pop(next_hash)
        branch.append(payload)
        next_hash = payload[4:36]
    utxos = {}
    for payload in list(reversed(branch))[1:]:  # Genesis coinbase is not in the UTXO view.
        offset = 112 if struct.unpack("<i", payload[:4])[0] > 3 else 80
        block = io.BytesIO(payload)
        block.seek(offset)
        for _ in range(compact(block)):
            txid, inputs, outputs = transaction(block)
            for h, n in inputs:
                if h == genesis_parent and n == 0xffffffff:
                    continue  # Coinbase.
                del utxos[h, n]
            for n, (value, script) in enumerate(outputs):
                if script[:1] != b"\x6a":  # OP_RETURN is immediately pruned.
                    utxos[txid, n] = value
    return len(branch) - 1, Counter(utxos.values())


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("snapshot", type=Path)
    cli.add_argument("replay", type=Path)
    cli.add_argument("multihash", type=Path)
    cli.add_argument("--candidate-bits", nargs="+", default=("1e0fffff", "1c04bc95", "1c023000", "1c010000"))
    args = cli.parse_args()
    ref = json.loads(args.replay.read_text(), parse_float=Decimal)
    height, distribution = collect(args.snapshot, args.multihash, ref["tip"])
    amounts = sum(amount * count for amount, count in distribution.items())
    assert height == ref["height"] and sum(distribution.values()) == ref["utxo"]["txouts"]
    assert amounts == Decimal(ref["utxo"]["total_amount"]) * 100_000_000
    max_output = max(distribution)
    hash_range = 1 << 256
    reports = []
    for raw in args.candidate_bits:
        target = decode_target(int(raw, 16))
        saturated = sum(count for n, count in distribution.items() if (n // 100) * target >= hash_range)
        max_hit = min(hash_range, (max_output // 100) * target)
        reports.append({"bits": raw, "saturated_outputs": saturated,
                        "largest_output_single_hash_probability": str(Decimal(max_hit) / hash_range)})
    print(json.dumps({"height": height, "outputs": sum(distribution.values()),
                      "largest_output_agr": str(Decimal(max_output) / 100_000_000),
                      "scenarios": reports}, indent=2))


if __name__ == "__main__":
    main()
