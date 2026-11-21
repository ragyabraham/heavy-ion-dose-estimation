# Carbon Ion PET Dose Estimation - Simulation Pipeline
## Geant4 (C++ core) + Python control, validation & visualisation

Matches Rutherford et al., Phys. Med. Biol. 67 (2022) 194001 parameters.
Generates 2-D (depth × lateral) dose and positron-emitter maps for subsequent ML training.

### Requirements
- Geant4 11.x (latest stable) with multi-threading
- CMake ≥ 3.16
- C++17 compiler
- Python ≥ 3.9 with numpy, matplotlib, h5py (or uproot/ROOT), pandas, pyyaml

### Build (on a machine with Geant4)
```bash
cd geant4_app
mkdir build && cd build
cmake .. -DGeant4_DIR=/path/to/geant4/lib/Geant4-11.x.x
make -j$(nproc)
```

### Quick test of analysis pipelines (no Geant4 required)
```bash
cd /workspace/artifacts/carbon_pet_pipeline
python3 -m tests.test_validation_and_viz
```

### Python control example
```python
from python_control.simulation import CarbonPETSimulation
sim = CarbonPETSimulation(config="configs/default.yaml")
sim.run_monoenergetic(energies=[200, 250, 300, 350])
```

### Project layout
- `geant4_app/`     : C++ Geant4 application (multi-threaded)
- `python_control/` : High-level experiment driver
- `validation/`     : Sanity checks and quantitative metrics
- `visualisation/`  : Depth-dose, 2-D maps, spatio-temporal plots
- `configs/`        : YAML run configurations
- `tests/`          : Self-contained tests with synthetic data
