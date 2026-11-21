"""
Self-contained test of the validation and visualisation pipelines.
Generates synthetic 2D data that mimics mono-energetic carbon-ion results
and exercises all sanity checks and plots. No Geant4 required.
"""

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import numpy as np
from validation.validator import SimulationValidator
from visualisation.visualizer import SimulationVisualizer


def make_synthetic_monoenergetic(energy_mev_u: float = 250.0,
                                 n_primaries: int = 1_000_000,
                                 n_depth: int = 200,
                                 n_lateral: int = 41) -> dict:
    """Create realistic-looking synthetic dose and activity maps."""
    z_mm = np.linspace(0, 200, n_depth)
    x_mm = np.linspace(-20, 20, n_lateral)

    # Approximate range (rough CSDA-like scaling for carbon in PMMA)
    # Tuned so 250 MeV/u ≈ 130 mm (realistic order of magnitude)
    range_mm = 0.45 * energy_mev_u * (energy_mev_u / 200) ** 0.6

    # Depth-dose: entrance plateau + Bragg peak
    depth_dose_1d = np.exp(-z_mm / (range_mm * 1.8))
    peak = np.exp(-0.5 * ((z_mm - range_mm) / 3.5) ** 2)
    depth_dose_1d = depth_dose_1d + 4.0 * peak
    depth_dose_1d /= depth_dose_1d.max()

    # Lateral Gaussian beam (8 mm FWHM ≈ σ ≈ 3.4 mm)
    sigma = 3.4
    lateral = np.exp(-0.5 * (x_mm / sigma) ** 2)

    dose_2d = np.outer(depth_dose_1d, lateral)
    # Tiny symmetric noise so symmetry check can pass
    noise = 0.005 * np.random.randn(*dose_2d.shape)
    noise = 0.5 * (noise + np.flip(noise, axis=1))
    dose_2d += noise
    dose_2d = np.clip(dose_2d, 0, None)

    # Activity roughly follows dose but shifted upstream and broader
    activity_1d = np.exp(-0.5 * ((z_mm - range_mm * 0.85) / 8) ** 2)
    activity_2d = np.outer(activity_1d, lateral * 1.2)
    activity_2d = np.clip(activity_2d, 0, None)

    return {
        "dose_2d": dose_2d.astype(np.float32),
        "activity_2d": activity_2d.astype(np.float32),
        "depth_dose": depth_dose_1d.astype(np.float32),
        "z_mm": z_mm.astype(np.float32),
        "x_mm": x_mm.astype(np.float32),
    }, {
        "energy_mev_u": energy_mev_u,
        "n_primaries": n_primaries,
        "expected_range_mm": range_mm,
    }


def main():
    print("=== CarbonPET Pipeline Self-Test ===\n")

    # Generate synthetic data for one energy
    data, meta = make_synthetic_monoenergetic(250.0)
    print(f"Synthetic data: dose_2d shape {data['dose_2d'].shape}, "
          f"expected range {meta['expected_range_mm']:.1f} mm")

    # Save for inspection
    out = Path("output/synthetic")
    out.mkdir(parents=True, exist_ok=True)
    np.savez(out / "mono_250MeV.npz", **data)
    print(f"Saved synthetic data to {out / 'mono_250MeV.npz'}")

    # Validation
    print("\n--- Validation ---")
    validator = SimulationValidator()
    report = validator.run_all(data, meta)
    print(f"Overall passed: {report['passed']}")
    for name, check in report["checks"].items():
        status = "PASS" if check["passed"] else "FAIL"
        extra = {k: v for k, v in check.items() if k != "passed"}
        print(f"  {name}: {status}  {extra}")

    validator.save_report(out / "validation_report.json")

    # Visualisation
    print("\n--- Visualisation ---")
    viz = SimulationVisualizer(output_dir=Path("output/figures"))
    viz.generate_summary(data, meta, prefix="synthetic_250MeV")

    print("\n=== Self-test complete ===")
    print("Inspect output/figures/ and output/synthetic/validation_report.json")


if __name__ == "__main__":
    main()
