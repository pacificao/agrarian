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


@dataclass(frozen=True)
class ReferenceScore:
    """Fixed calibration for the offline candidate; not mainnet parameters.

    The exact numerator orders the same way as work/work_ref + stake/stake_ref.
    No division or rounding happens when recording a block or comparing tips.
    """

    work_ref: int
    stake_ref: int

    def __post_init__(self):
        if not all(0 < ref < 1 << 256 for ref in (self.work_ref, self.stake_ref)):
            raise ValueError("reference must be a positive uint256")

    def score(self, totals: ProofTotals) -> int:
        return totals.work * self.stake_ref + totals.stake * self.work_ref

    def compare(self, left: ProofTotals, right: ProofTotals) -> int:
        """Compare evidence only; equal score does not decide the active tip."""
        lhs, rhs = self.score(left), self.score(right)
        return (lhs > rhs) - (lhs < rhs)


STAGING_SCORE = ReferenceScore(target_work(WORK_BITS), target_work(STAKE_BITS))


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

    def test_reference_score_resolves_the_measured_unit_mismatch(self):
        old = ReferenceScore(1, 1)
        work = ProofTotals().append("pow", WORK_BITS)
        stake = ProofTotals().append("pos", STAKE_BITS)
        work_24 = ProofTotals(24 * work.work, 0)
        self.assertEqual(old.compare(work_24, stake), -1)
        self.assertEqual(STAGING_SCORE.compare(work, stake), 0)
        self.assertEqual(STAGING_SCORE.compare(work_24, stake), 1)
        self.assertEqual(STAGING_SCORE.compare(ProofTotals(0, 24 * stake.stake), work), 1)

    def test_mixed_branch_strengthens_shared_history_without_a_quota(self):
        path = [ProofTotals()]
        for proof, bits in [("pow", WORK_BITS)] * 30 + [("pos", STAKE_BITS)] * 30 + [("pow", WORK_BITS)]:
            path.append(path[-1].append(proof, bits))
        self.assertTrue(all(STAGING_SCORE.compare(new, old) == 1
                            for old, new in zip(path, path[1:])))
        work = ProofTotals(30 * STAGING_SCORE.work_ref, 0)
        stake = ProofTotals(0, 30 * STAGING_SCORE.stake_ref)
        self.assertEqual(STAGING_SCORE.compare(path[-1], work), 1)
        self.assertEqual(STAGING_SCORE.compare(path[-1], stake), 1)

    def test_candidate_handles_unequal_targets_without_rounding_away_evidence(self):
        root = ProofTotals(1 << 270, 1 << 270)
        easy_bits = 0x2100FFFF  # valid at the uint256 level, not a network limit
        self.assertEqual(target_work(easy_bits), 1)
        for proof in ("pow", "pos"):
            candidate = root.append(proof, easy_bits)
            self.assertEqual(STAGING_SCORE.compare(candidate, root), 1)
        self.assertEqual(STAGING_SCORE.compare(root.append("pow", WORK_BITS - 1),
                                               root.append("pos", STAKE_BITS)), 1)
        self.assertEqual(STAGING_SCORE.compare(root.append("pow", WORK_BITS + 1),
                                               root.append("pos", STAKE_BITS)), -1)

    def test_candidate_is_independent_of_common_prefix_and_input_order(self):
        def best(branches):
            return max(branches, key=STAGING_SCORE.score)

        work, stake = STAGING_SCORE.work_ref, STAGING_SCORE.stake_ref
        left, right, mixed = ProofTotals(4 * work, 0), ProofTotals(0, 3 * stake), ProofTotals(work, 4 * stake)
        self.assertEqual(best([left, right, mixed]), mixed)
        self.assertEqual(best([mixed, right, left]), mixed)
        prefix = ProofTotals(8 * work, 9 * stake)
        with_prefix = [ProofTotals(t.work + prefix.work, t.stake + prefix.stake)
                       for t in [left, right, mixed]]
        self.assertEqual(best(with_prefix), with_prefix[-1])

    def test_calibration_sensitivity_is_explicit(self):
        work, stake = STAGING_SCORE.work_ref, STAGING_SCORE.stake_ref
        work_tail, stake_tail = ProofTotals(3 * work, 0), ProofTotals(0, 2 * stake)
        self.assertEqual(STAGING_SCORE.compare(work_tail, stake_tail), 1)
        self.assertEqual(ReferenceScore(2 * work, stake).compare(work_tail, stake_tail), -1)
        self.assertEqual(ReferenceScore(work, 2 * stake).compare(work_tail, stake_tail), 1)
        for bad in (0, -1, 1 << 256):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                ReferenceScore(work, bad)


if __name__ == "__main__":
    unittest.main()
