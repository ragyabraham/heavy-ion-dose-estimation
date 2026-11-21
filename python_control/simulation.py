"""
High-level Python control interface for CarbonPET simulations.
"""

from pathlib import Path
from typing import Dict, List, Optional, Sequence
import os
import subprocess
import yaml
import numpy as np

from python_control.maps import crop_lateral, scoring_to_maps

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GEANT4_SH = Path("/Users/ragy/geant4-install/bin/geant4.sh")


class CarbonPETSimulation:
    def __init__(self, config: str = "configs/default.yaml"):
        with open(config) as f:
            self.config = yaml.safe_load(f)
        self.output_dir = Path(self.config["output"]["directory"])
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def geant4_binary(self) -> Path:
        binary = REPO_ROOT / "geant4_app" / "build" / "carbon_pet_sim"
        if not binary.is_file():
            raise FileNotFoundError(f"Geant4 app not built: {binary}")
        return binary

    def write_scan_macro(self, energies: Sequence[float], n_primaries: int) -> Path:
        """One Geant4 session, a fresh scoring file for each energy."""
        spread = float(self.config["beam"]["energy_spread"])
        lines = [
            "/run/initialize",
            f"/carbon/beam/energySpread {spread}",
        ]
        for energy in energies:
            lines += [
                f"/carbon/beam/energyPerNucleon {energy:g}",
                f"/carbon/scoring/file scoring_{energy:g}.txt",
                "/carbon/scoring/reset",
                f"/run/beamOn {int(n_primaries)}",
            ]
        path = self.output_dir / "energy_scan.mac"
        path.write_text("\n".join(lines) + "\n")
        return path

    def run_monoenergetic(self, energies: Optional[List[float]] = None,
                          n_primaries: Optional[int] = None,
                          make_figures: bool = True) -> List[dict]:
        """
        Run the configured energy list and export one npz per energy.

        n_primaries is required. configs/default.yaml lists 1e8 histories,
        which is the production figure, not this check.
        """
        if n_primaries is None or n_primaries < 1:
            raise ValueError("n_primaries must be a positive integer")
        energies = list(energies or self.config["energy_mev_u"])
        binary = self.geant4_binary()
        geant4_sh = Path(os.environ.get("GEANT4_SH", DEFAULT_GEANT4_SH))
        if not geant4_sh.is_file():
            raise FileNotFoundError(f"Geant4 environment script not found: {geant4_sh}")

        macro = self.write_scan_macro(energies, n_primaries)
        log_path = self.output_dir / "energy_scan.log"
        print(f"Running {len(energies)} energies, {n_primaries} primaries each")
        print(f"  macro {macro}")
        print(f"  log   {log_path}")
        command = f'source "{geant4_sh}" && "{binary}" "{macro.name}"'
        with open(log_path, "w") as log:
            subprocess.run(
                ["bash", "-c", command],
                cwd=self.output_dir,
                stdout=log,
                stderr=subprocess.STDOUT,
                check=True,
            )

        rows = []
        for energy in energies:
            scoring = self.output_dir / f"scoring_{energy:g}.txt"
            if not scoring.is_file():
                raise FileNotFoundError(f"Missing scoring file: {scoring}")
            data = self.export_2d_maps(
                scoring, energy_mev_u=energy, make_figures=make_figures
            )
            peak_z = float(data["z_mm"][int(np.argmax(data["depth_dose"]))])
            rows.append({
                "energy_mev_u": energy,
                "peak_depth_mm": peak_z,
                "total_mev": float(data["depth_dose"].sum()),
                "activity_counts": float(data["activity_2d"].sum()),
            })
        self.plot_range_scan(rows)
        return rows

    def plot_range_scan(self, rows: Sequence[dict]):
        from matplotlib import pyplot as plt

        energies = [row["energy_mev_u"] for row in rows]
        peaks = [row["peak_depth_mm"] for row in rows]
        fig, ax = plt.subplots(figsize=(7, 5))
        ax.plot(energies, peaks, "o-", lw=2)
        ax.set_xlabel("Beam energy (MeV/u)")
        ax.set_ylabel("Depth of maximum dose (mm)")
        ax.set_title("Mono-energetic carbon range in PMMA")
        ax.grid(True, alpha=0.3)
        figure_dir = self.output_dir.parent / "figures"
        figure_dir.mkdir(parents=True, exist_ok=True)
        path = figure_dir / "energy_scan_range.png"
        fig.savefig(path, dpi=150, bbox_inches="tight")
        plt.close(fig)
        print(f"Wrote {path}")
        print(f"{'MeV/u':>8} {'peak mm':>10} {'dose MeV':>12} {'emitters':>10}")
        for row in rows:
            print(f"{row['energy_mev_u']:8g} {row['peak_depth_mm']:10.1f} "
                  f"{row['total_mev']:12.1f} {row['activity_counts']:10.0f}")

    def export_2d_maps(self, scoring_path: str,
                       energy_mev_u: float = 250.0,
                       lateral_half_width_mm: float = 25.0,
                       make_figures: bool = True) -> Dict[str, np.ndarray]:
        """Project scoring_results.txt onto depth × lateral and write an npz."""
        scoring_path = Path(scoring_path)
        data = scoring_to_maps(scoring_path)
        tag = f"mono_{energy_mev_u:g}MeV"
        out_path = self.output_dir / f"{tag}.npz"
        np.savez(out_path, **data)

        depth = data["depth_dose"]
        peak_z = float(data["z_mm"][int(np.argmax(depth))])
        total_mev = float(depth.sum())
        activity_counts = float(data["activity_2d"].sum())
        print(f"Wrote {out_path}")
        print(f"  dose voxels in file projected to {data['dose_2d'].shape} (z, x)")
        print(f"  total scored dose {total_mev:.1f} MeV")
        print(f"  depth of maximum dose {peak_z:.1f} mm")
        print(f"  positron-emitter counts {activity_counts:.0f}")

        if make_figures:
            from matplotlib import pyplot as plt
            from visualisation.visualizer import SimulationVisualizer

            view = crop_lateral(data, lateral_half_width_mm)
            # Depth-dose stays the integral over the whole transverse plane.
            view["depth_dose"] = data["depth_dose"]
            view["z_mm"] = data["z_mm"]
            viz = SimulationVisualizer(output_dir=self.output_dir.parent / "figures")
            viz.generate_summary(
                view,
                {"energy_mev_u": energy_mev_u, "dose_ylabel": "Scored energy (MeV)"},
                prefix=tag)
            plt.close("all")
        return data


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser(description="CarbonPET simulation control")
    parser.add_argument("--config", default="configs/default.yaml")
    sub = parser.add_subparsers(dest="cmd", required=True)

    export = sub.add_parser("export", help="Project one scoring_results.txt into 2D maps")
    export.add_argument("scoring")
    export.add_argument("--energy-mev-u", type=float, default=250.0)

    scan = sub.add_parser("scan", help="Run the mono-energetic energy list")
    scan.add_argument("--n-primaries", type=int, default=50,
                      help="Histories per energy. The yaml value 1e8 is the production run.")
    scan.add_argument("--no-figures", action="store_true")

    # Keep the earlier form: simulation.py <scoring.txt> --energy-mev-u 250
    argv = sys.argv[1:]
    if argv and argv[0] not in ("export", "scan", "-h", "--help"):
        argv = ["export", *argv]
    args = parser.parse_args(argv)
    sim = CarbonPETSimulation(config=args.config)
    if args.cmd == "export":
        sim.export_2d_maps(args.scoring, energy_mev_u=args.energy_mev_u)
    else:
        sim.run_monoenergetic(
            n_primaries=args.n_primaries,
            make_figures=not args.no_figures,
        )
