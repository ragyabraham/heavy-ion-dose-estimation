"""
Validation pipeline for CarbonPET simulation outputs.
Runs quantitative sanity checks with clear pass/fail metrics.
"""

import numpy as np
from pathlib import Path
from typing import Dict, Any, Tuple
import json


class SimulationValidator:
    def __init__(self, dose_voxel_mm: float = 1.0, activity_voxel_mm: float = 1.5):
        self.dose_voxel_mm = dose_voxel_mm
        self.activity_voxel_mm = activity_voxel_mm
        self.results = {}

    def load_raw(self, path: Path) -> Dict[str, np.ndarray]:
        """Load raw scored data. Supports .npy or .npz for testing; extend for ROOT/HDF5."""
        path = Path(path)
        if path.suffix == ".npz":
            data = np.load(path)
            return {k: data[k] for k in data.files}
        elif path.suffix == ".npy":
            return {"dose": np.load(path)}
        else:
            raise ValueError(f"Unsupported format: {path}")

    def check_energy_conservation(self, dose_map: np.ndarray, n_primaries: int,
                                  energy_per_primary_mev: float,
                                  tolerance: float = 0.05) -> Tuple[bool, float]:
        """Integral of dose should match total energy deposited (approx)."""
        # Rough conversion: dose (Gy) * mass; here we use relative integral
        total_dose_integral = np.sum(dose_map)
        expected = n_primaries * energy_per_primary_mev * 1e-3  # placeholder scaling
        rel_error = abs(total_dose_integral - expected) / (expected + 1e-12)
        passed = rel_error < tolerance
        return passed, rel_error

    def check_bragg_peak(self, depth_dose: np.ndarray, z_mm: np.ndarray,
                         expected_range_mm: float, tolerance_mm: float = 2.0) -> Tuple[bool, float]:
        """Distal edge (80% of peak) within tolerance of expected range."""
        peak_idx = np.argmax(depth_dose)
        peak_val = depth_dose[peak_idx]
        # Find 80% distal fall-off
        distal = depth_dose[peak_idx:]
        z_distal = z_mm[peak_idx:]
        below = np.where(distal < 0.8 * peak_val)[0]
        if len(below) == 0:
            edge_z = z_mm[-1]
        else:
            edge_z = z_distal[below[0]]
        error_mm = abs(edge_z - expected_range_mm)
        passed = error_mm <= tolerance_mm
        return passed, error_mm

    def check_no_negatives(self, *arrays) -> bool:
        return all(np.all(a >= -1e-6) for a in arrays)

    def check_symmetry(self, map_2d: np.ndarray, axis: int = 1, tolerance: float = 0.15) -> Tuple[bool, float]:
        """Left-right symmetry of 2D map."""
        flipped = np.flip(map_2d, axis=axis)
        diff = np.mean(np.abs(map_2d - flipped)) / (np.mean(np.abs(map_2d)) + 1e-12)
        return diff < tolerance, diff

    def run_all(self, data: Dict[str, np.ndarray], metadata: Dict[str, Any]) -> Dict[str, Any]:
        report = {"passed": True, "checks": {}}

        dose = data.get("dose_2d")
        if dose is None:
            report["passed"] = False
            report["error"] = "Missing dose_2d"
            return report

        # Negatives
        neg_ok = self.check_no_negatives(dose)
        report["checks"]["no_negatives"] = {"passed": neg_ok}

        # Symmetry
        sym_ok, sym_diff = self.check_symmetry(dose)
        report["checks"]["symmetry"] = {"passed": sym_ok, "mean_rel_diff": float(sym_diff)}

        # Bragg peak / range (if 1D projection available)
        if "depth_dose" in data and "z_mm" in data:
            bp_ok, bp_err = self.check_bragg_peak(
                data["depth_dose"], data["z_mm"],
                metadata.get("expected_range_mm", 150.0)
            )
            report["checks"]["bragg_peak"] = {"passed": bp_ok, "error_mm": float(bp_err)}

        # Energy conservation (simplified)
        if "n_primaries" in metadata and "energy_mev_u" in metadata:
            # Approximate
            ec_ok, ec_err = self.check_energy_conservation(
                dose, metadata["n_primaries"], metadata["energy_mev_u"] * 12  # rough
            )
            report["checks"]["energy_conservation"] = {"passed": ec_ok, "rel_error": float(ec_err)}

        report["passed"] = all(bool(c.get("passed", False)) for c in report["checks"].values())
        # Ensure JSON-serializable
        def make_serializable(obj):
            if isinstance(obj, dict):
                return {k: make_serializable(v) for k, v in obj.items()}
            if isinstance(obj, (np.bool_, np.integer, np.floating)):
                return obj.item()
            return obj
        report = make_serializable(report)
        self.results = report
        return report

    def save_report(self, path: Path):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w") as f:
            json.dump(self.results, f, indent=2)
        print(f"Validation report written to {path}")
