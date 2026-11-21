"""
Visualisation pipeline for CarbonPET 2D dose and activity maps.
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from typing import Optional, Dict


class SimulationVisualizer:
    def __init__(self, output_dir: Path = Path("output/figures")):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        plt.style.use("seaborn-v0_8-whitegrid")

    def plot_depth_dose(self, z_mm: np.ndarray, depth_dose: np.ndarray,
                        title: str = "Depth-Dose", distal_edge: Optional[float] = None,
                        save_as: Optional[str] = None,
                        ylabel: str = "Dose (a.u.)"):
        fig, ax = plt.subplots(figsize=(8, 5))
        ax.plot(z_mm, depth_dose, "b-", lw=2, label="Dose")
        if distal_edge is not None:
            ax.axvline(distal_edge, color="r", ls="--", label=f"Distal edge ({distal_edge:.1f} mm)")
        ax.set_xlabel("Depth (mm)")
        ax.set_ylabel(ylabel)
        ax.set_title(title)
        ax.legend()
        ax.grid(True, alpha=0.3)
        if save_as:
            fig.savefig(self.output_dir / save_as, dpi=150, bbox_inches="tight")
        return fig

    def plot_2d_map(self, map_2d: np.ndarray, z_mm: np.ndarray, x_mm: np.ndarray,
                    title: str = "2D Map", cmap: str = "viridis",
                    save_as: Optional[str] = None):
        fig, ax = plt.subplots(figsize=(10, 6))
        extent = [x_mm[0], x_mm[-1], z_mm[0], z_mm[-1]]
        # map_2d is (depth, lateral): rows are z, columns are x.
        im = ax.imshow(map_2d, origin="lower", extent=extent, aspect="auto", cmap=cmap)
        ax.set_xlabel("Lateral position (mm)")
        ax.set_ylabel("Depth (mm)")
        ax.set_title(title)
        plt.colorbar(im, ax=ax, label="Intensity")
        if save_as:
            fig.savefig(self.output_dir / save_as, dpi=150, bbox_inches="tight")
        return fig

    def plot_dose_activity_overlay(self, dose_2d: np.ndarray, activity_2d: np.ndarray,
                                   z_mm: np.ndarray, x_mm: np.ndarray,
                                   save_as: Optional[str] = None,
                                   activity_z_mm: Optional[np.ndarray] = None,
                                   activity_x_mm: Optional[np.ndarray] = None):
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        extent = [x_mm[0], x_mm[-1], z_mm[0], z_mm[-1]]
        activity_z_mm = z_mm if activity_z_mm is None else activity_z_mm
        activity_x_mm = x_mm if activity_x_mm is None else activity_x_mm
        activity_extent = [activity_x_mm[0], activity_x_mm[-1],
                           activity_z_mm[0], activity_z_mm[-1]]

        im0 = axes[0].imshow(dose_2d, origin="lower", extent=extent, aspect="auto", cmap="hot")
        axes[0].set_title("Dose")
        plt.colorbar(im0, ax=axes[0])

        im1 = axes[1].imshow(activity_2d, origin="lower", extent=activity_extent,
                             aspect="auto", cmap="Blues")
        axes[1].set_title("Activity (positrons)")
        plt.colorbar(im1, ax=axes[1])

        axes[2].imshow(dose_2d, origin="lower", extent=extent, aspect="auto", cmap="hot", alpha=0.7)
        if np.any(activity_2d > 0):
            axes[2].contour(activity_x_mm, activity_z_mm, activity_2d,
                            levels=5, colors="cyan", linewidths=1)
        axes[2].set_title("Dose + Activity contours")

        for ax in axes:
            ax.set_xlabel("Lateral (mm)")
            ax.set_ylabel("Depth (mm)")
        fig.tight_layout()
        if save_as:
            fig.savefig(self.output_dir / save_as, dpi=150, bbox_inches="tight")
        return fig

    def generate_summary(self, data: Dict[str, np.ndarray], metadata: Dict, prefix: str = "run"):
        """Generate standard set of figures for one energy."""
        z = data.get("z_mm", np.arange(data["dose_2d"].shape[0]))
        x = data.get("x_mm", np.linspace(-20, 20, data["dose_2d"].shape[1]))

        self.plot_depth_dose(z, data.get("depth_dose", data["dose_2d"].sum(axis=1)),
                             title=f"Depth-Dose ({metadata.get('energy_mev_u', '?')} MeV/u)",
                             ylabel=metadata.get("dose_ylabel", "Dose (a.u.)"),
                             save_as=f"{prefix}_depth_dose.png")

        self.plot_2d_map(data["dose_2d"], z, x,
                         title=f"2D Dose ({metadata.get('energy_mev_u', '?')} MeV/u)",
                         save_as=f"{prefix}_dose_2d.png")

        if "activity_2d" in data:
            za = data.get("activity_z_mm", z)
            xa = data.get("activity_x_mm", x)
            self.plot_2d_map(data["activity_2d"], za, xa,
                             title=f"2D Activity ({metadata.get('energy_mev_u', '?')} MeV/u)",
                             cmap="Blues", save_as=f"{prefix}_activity_2d.png")
            self.plot_dose_activity_overlay(
                data["dose_2d"], data["activity_2d"], z, x,
                activity_z_mm=za, activity_x_mm=xa,
                save_as=f"{prefix}_overlay.png")

        print(f"Figures saved to {self.output_dir}")
