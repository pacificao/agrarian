#!/usr/bin/env python3
"""Offline fork-choice design checks. This module is not a consensus rule."""

import unittest
from dataclasses import dataclass
from fractions import Fraction


def decode_target(bits: int) -> int:
    if not 0 <= bits <= 0xFFFFFFFF or bits & 0x00800000:
        raise ValueError("invalid compact target")
    exponent, mantissa = bits >> 24, bits & 0x007FFFFF
    shift = 8 * (exponent - 3)
    target = mantissa << shift if shift >= 0 else mantissa >> -shift
    if not 0 < target < 1 << 256:
        raise ValueError("target is outside the nonzero uint256 range")
    return target


def target_work(bits: int) -> int:
    """Mirror GetBlockProof for valid compact targets used by these fixtures."""
    return (1 << 256) // (decode_target(bits) + 1)


@dataclass(frozen=True)
class ProofTotals:
    """Evidence since a common ancestor, without converting between proofs."""

    work: int = 0
    stake: int = 0

    def append(self, proof: str, bits: int):
        score = target_work(bits)
        if proof == "pow":
            return ProofTotals(self.work + score, self.stake)
        if proof == "pos":
            return ProofTotals(self.work, self.stake + score)
        raise ValueError("unknown proof")

    def compare_components(self, other):
        """A partial order; unlike a fork choice, it can report incomparable."""
        left = self.work - other.work
        right = self.stake - other.stake
        if left == right == 0:
            return "equal"
        if left >= 0 and right >= 0:
            return "greater"
        if left <= 0 and right <= 0:
            return "less"
        return "incomparable"

    def break_even_stake_multiplier(self, other):
        """Inspect weight sensitivity, not a choice of production weights."""
        work_delta = self.work - other.work
        stake_delta = self.stake - other.stake
        if not work_delta or not stake_delta or (work_delta > 0) == (stake_delta > 0):
            return None
        return Fraction(-work_delta, stake_delta)


# Proposed first retarget results on already validated mainnet block 8,889.
WORK_BITS = 0x1D173C32
STAKE_BITS = 0x1C04BC95


class HybridBranchModelTest(unittest.TestCase):
    def test_either_stream_adds_evidence_without_the_other(self):
        root = ProofTotals()
        for proof, bits in (("pow", WORK_BITS), ("pos", STAKE_BITS)):
            tip = root.append(proof, bits)
            self.assertEqual(tip.compare_components(root), "greater")
            self.assertEqual(tip.append(proof, bits).compare_components(tip), "greater")

    def test_returning_stream_adds_evidence_without_erasing_first_stream(self):
        work = ProofTotals().append("pow", WORK_BITS)
        stake = ProofTotals().append("pos", STAKE_BITS)
        mixed = work.append("pos", STAKE_BITS)
        self.assertEqual(mixed, stake.append("pow", WORK_BITS))
        self.assertEqual(mixed.compare_components(work), "greater")
        self.assertEqual(mixed.compare_components(stake), "greater")

    def test_disjoint_branches_expose_the_tradeoff_not_a_fake_comparison(self):
        work = ProofTotals().append("pow", WORK_BITS)
        stake = ProofTotals().append("pos", STAKE_BITS)
        self.assertEqual(work.compare_components(stake), "incomparable")
        self.assertEqual(stake.compare_components(work), "incomparable")
        self.assertEqual(work.break_even_stake_multiplier(stake),
                         Fraction(184847936, 232128606112))
        self.assertGreater(Fraction(stake.stake, work.work), 1255)
        self.assertLess(Fraction(stake.stake, work.work), 1257)

    def test_invalid_targets_do_not_enter_the_model(self):
        for bits in (0, 0x01800001, 0x23010000, -1):
            with self.subTest(bits=bits), self.assertRaises(ValueError):
                target_work(bits)

    def test_full_width_stake_target_must_not_wrap(self):
        target = decode_target(0x207FFFFF)
        domain = 1 << 256
        # A strictly wider range of valid hashes for a larger weight must
        # saturate at the hash-domain size rather than wrap back down.
        self.assertLess((3 * target) % domain, target)
        self.assertEqual(min(3 * target, domain), domain)
        self.assertGreaterEqual(min(3 * target, domain), min(2 * target, domain))


if __name__ == "__main__":
    unittest.main()
