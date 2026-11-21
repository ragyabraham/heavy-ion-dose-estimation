"""
High-level Python control interface for CarbonPET simulations.
"""

from pathlib import Path
from typing import List, Optional
import yaml
import numpy as np


class CarbonPETSimulation:
    def __init__(self, config: str = "configs/default.yaml"):
        with open(config) as f:
            self.config = yaml.safe_load(f)
        self.output_dir = Path(self.config["output"]["directory"])
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def run_monoenergetic(self, energies: Optional[List[float]] = None,
                          n_primaries: Optional[int] = None):
        """
        Launch mono-energetic runs.
        In full deployment this calls the Geant4 executable or Python bindings.
        Here it documents the interface and can generate placeholder outputs.
        """
        energies = energies or self.config["energy_mev_u"]
        n_primaries = n_primaries or self.config["n_primaries"]
        print(f"Scheduling {len(energies)} mono-energetic runs "
              f"({n_primaries:.1e} primaries each)")
        for e in energies:
            print(f"  → {e} MeV/u")
            # Placeholder: in production, subprocess.call(["./carbon_pet_sim", macro])
            # or direct pybind11 call
        print("Runs complete (placeholder). Raw data would appear in", self.output_dir)

    def export_2d_maps(self, roi_radius_mm: Optional[float] = None):
        """Derive depth × lateral maps from raw 3-D scored data."""
        print("Exporting 2-D maps (depth × lateral)…")
        # Implementation would read ROOT/HDF5 and write npz/hdf5 of 2-D arrays
        print("Done.")


if __name__ == "__main__":
    sim = CarbonPETSimulation()
    sim.run_monoenergetic(energies=[200, 250, 300])
