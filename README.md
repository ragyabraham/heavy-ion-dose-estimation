# Carbon Ion PET Dose Estimation - Simulation Pipeline

Geant4 C++ application with a Python layer for control and plots. The setup follows Rutherford et al., Phys. Med. Biol. 67 (2022) 194001, and produces 2-D (depth × lateral) dose and positron-emitter maps for ¹²C in PMMA.

The beam energy used in the physics is the total kinetic energy. The names and the config use MeV per nucleon. For ¹²C those differ by a factor of 12, so 250 MeV/u is 3000 MeV total. The default gun is 250 MeV/u.

## Requirements

- Geant4 11.3 with the Qt visualization drivers (this machine: `/Users/ragy/geant4-install`)
- CMake ≥ 3.16 and a C++17 compiler
- Python 3.11 or newer with `numpy`, `matplotlib`, and `pyyaml` (the system `python3` on this Mac does not have them)

## Build

```bash
source /Users/ragy/geant4-install/bin/geant4.sh
cd geant4_app
mkdir -p build && cd build
cmake .. -DGeant4_DIR=/Users/ragy/geant4-install/lib/cmake/Geant4
make -j$(sysctl -n hw.ncpu)
```

On Linux, `nproc` replaces the `sysctl` command.

## Run one energy

From `geant4_app/build`, after sourcing the Geant4 setup:

```bash
./carbon_pet_sim
```

In the Qt session:

```text
/run/beamOn 1000
```

The startup line `Primary particle set to: C12  250 MeV/u  (3000 MeV total before spread)` is the default beam. When `/run/beamOn` finishes, the app writes `scoring_results.txt` in the working directory. A second `/run/beamOn` in the same session adds to those maps. `exit` closes the viewer.

To change the energy before the run, use MeV per nucleon:

```text
/carbon/beam/energyPerNucleon 290
/carbon/beam/energySpread 0.002
/run/beamOn 1000
```

`0.002` is a relative Gaussian width of 0.2%. Batch mode takes a macro. The macro must call `/run/initialize` itself:

```text
/run/initialize
/carbon/beam/energyPerNucleon 290
/carbon/scoring/file scoring_290.txt
/carbon/scoring/reset
/run/beamOn 1000
```

```bash
./carbon_pet_sim run.mac
```

`/carbon/scoring/reset` clears the maps. `/carbon/scoring/file` sets the name written at the end of the next run. Use a filename with no spaces.

Radioactive decay is turned off at the start of each run. Geant4 11.3 aborts if a stopped fragment decays into a nucleus that is already in the particle table. Positron emitters (¹¹C, ¹⁰C, ¹⁵O, ¹³N) are counted when they stop.

## Make the maps and figures

From the repository root:

```bash
MPLBACKEND=Agg python3.11 -m python_control.simulation export \
  geant4_app/build/scoring_results.txt \
  --energy-mev-u 250
```

This writes `output/raw/mono_250MeV.npz` and the figures in `output/figures/`. The dose array has shape `(300, 100)`: 300 depth bins and 100 lateral bins, each 1 mm. That shape is the target size, not an energy. Activity is scored on 1.5 mm voxels.

## Energy scan

`configs/default.yaml` lists 200 to 350 MeV/u in steps of 15. The value `n_primaries: 100000000` in that file is the production statistics and is not used automatically. This command runs the list with 50 histories per energy, which is enough to see the Bragg peak move:

```bash
MPLBACKEND=Agg python3.11 -m python_control.simulation scan --n-primaries 50
```

One Geant4 process runs every energy, resetting the maps between them. Results:

- `output/raw/scoring_<energy>.txt` for the raw voxels
- `output/raw/mono_<energy>MeV.npz` for the depth × lateral maps
- `output/figures/mono_<energy>MeV_*.png` for each energy
- `output/figures/energy_scan_range.png` for the depth of maximum dose against energy

On the 50-history scan the dose maximum moved from 74.5 mm at 200 MeV/u to 191.5 mm at 350 MeV/u.

From Python, pass the history count explicitly:

```python
from python_control.simulation import CarbonPETSimulation
sim = CarbonPETSimulation(config="configs/default.yaml")
sim.run_monoenergetic(n_primaries=50)
```

## Analysis check without Geant4

```bash
python3.11 tests/test_validation_and_viz.py
```

That test uses synthetic maps. It does not read `scoring_results.txt`.

## Layout

- `geant4_app/` — C++ application. The run manager is serial.
- `python_control/` — driver that launches the scan and builds the 2-D maps
- `validation/` — checks on saved map arrays
- `visualisation/` — depth-dose and 2-D figures
- `configs/default.yaml` — energy list, beam width, voxel sizes
- `output/raw/` and `output/figures/` — scan products
- `tests/` — synthetic-data check
