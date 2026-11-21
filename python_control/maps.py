"""Turn CarbonPET scoring_results.txt into depth × lateral maps.

The text file is written by CarbonPETScoring::WriteResults. Each index is
packed the same way as CarbonPETScoring::Index3D:

    index = iz * nx * nx + iy * nx + ix

with the target centred in x and y (±50 mm) and starting at z = 0.
Dose voxels are 1 mm. Activity voxels are 1.5 mm.
"""

from pathlib import Path
from typing import Dict, Tuple

import numpy as np

HALF_XY_MM = 50.0
LENGTH_Z_MM = 300.0
DOSE_VOXEL_MM = 1.0
ACTIVITY_VOXEL_MM = 1.5


def grid_shape(voxel_mm: float,
               half_xy_mm: float = HALF_XY_MM,
               length_z_mm: float = LENGTH_Z_MM) -> Tuple[int, int]:
    nx = max(1, int(np.floor((2.0 * half_xy_mm) / voxel_mm)))
    nz = max(1, int(np.floor(length_z_mm / voxel_mm)))
    return nx, nz


def bin_centres(n: int, voxel_mm: float, origin_mm: float) -> np.ndarray:
    return origin_mm + (np.arange(n) + 0.5) * voxel_mm


def load_scoring_txt(path: Path) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Return dose indices, dose MeV, activity indices, activity counts."""
    dose_idx = []
    dose_val = []
    act_idx = []
    act_val = []
    section = "dose"
    with open(path) as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                if "activity" in line.lower():
                    section = "activity"
                continue
            index_text, value_text = line.split()
            if section == "dose":
                dose_idx.append(int(index_text))
                dose_val.append(float(value_text))
            else:
                act_idx.append(int(index_text))
                act_val.append(float(value_text))
    return (
        np.asarray(dose_idx, dtype=np.int64),
        np.asarray(dose_val, dtype=np.float64),
        np.asarray(act_idx, dtype=np.int64),
        np.asarray(act_val, dtype=np.float64),
    )


def unpack(indices: np.ndarray, values: np.ndarray, voxel_mm: float) -> np.ndarray:
    """Scatter packed indexes into a volume shaped (nz, ny, nx)."""
    nx, nz = grid_shape(voxel_mm)
    volume = np.zeros((nz, nx, nx), dtype=np.float64)
    if indices.size == 0:
        return volume
    iz, rem = np.divmod(indices, nx * nx)
    iy, ix = np.divmod(rem, nx)
    valid = (
        (ix >= 0) & (ix < nx) &
        (iy >= 0) & (iy < nx) &
        (iz >= 0) & (iz < nz)
    )
    np.add.at(volume, (iz[valid], iy[valid], ix[valid]), values[valid])
    return volume


def project_depth_lateral(volume: np.ndarray, voxel_mm: float) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Sum along y. Returns map[z, x], z centres (mm), x centres (mm)."""
    nx = volume.shape[2]
    nz = volume.shape[0]
    projected = volume.sum(axis=1)
    z_mm = bin_centres(nz, voxel_mm, 0.0)
    x_mm = bin_centres(nx, voxel_mm, -HALF_XY_MM)
    return projected, z_mm, x_mm


def scoring_to_maps(path: Path) -> Dict[str, np.ndarray]:
    dose_idx, dose_val, act_idx, act_val = load_scoring_txt(path)
    dose_vol = unpack(dose_idx, dose_val, DOSE_VOXEL_MM)
    act_vol = unpack(act_idx, act_val, ACTIVITY_VOXEL_MM)
    dose_2d, z_mm, x_mm = project_depth_lateral(dose_vol, DOSE_VOXEL_MM)
    activity_2d, activity_z_mm, activity_x_mm = project_depth_lateral(
        act_vol, ACTIVITY_VOXEL_MM
    )
    depth_dose = dose_2d.sum(axis=1)
    return {
        "dose_2d": dose_2d.astype(np.float32),
        "activity_2d": activity_2d.astype(np.float32),
        "depth_dose": depth_dose.astype(np.float32),
        "z_mm": z_mm.astype(np.float32),
        "x_mm": x_mm.astype(np.float32),
        "activity_z_mm": activity_z_mm.astype(np.float32),
        "activity_x_mm": activity_x_mm.astype(np.float32),
    }


def crop_lateral(data: Dict[str, np.ndarray], half_width_mm: float = 25.0) -> Dict[str, np.ndarray]:
    """Keep |x| <= half_width_mm so the pencil beam fills the figure."""
    cropped = dict(data)
    for map_key, x_key in (("dose_2d", "x_mm"), ("activity_2d", "activity_x_mm")):
        x = data[x_key]
        keep = np.abs(x) <= half_width_mm
        cropped[map_key] = data[map_key][:, keep]
        cropped[x_key] = x[keep]
    return cropped
