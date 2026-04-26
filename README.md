# Unified TCP Co-Simulation Framework

This project presents one core idea:

Use Python as the external intelligence layer for power-system simulations, while keeping high-fidelity plant models in their native simulation tools.

In this repository, that idea is implemented for both MATLAB/Simulink and DIgSILENT PowerFactory through TCP-based real-time data exchange.

[![DOI](https://img.shields.io/badge/DOI-10.1109%2FKPEC58008.2023.10215460-blue)](https://doi.org/10.1109/KPEC58008.2023.10215460)

## Big Picture

### PowerFactory + Python

![PowerFactory TCP architecture](PowerFactory-Python/images/PowerFactory.svg)

### MATLAB/Simulink + Python

![MATLAB Simulink TCP architecture](PowerFactory-Python/images/Matlab.svg)

### Parallel MATLAB Instances for Faster Training

![Concurrent multi-instance MATLAB training](PowerFactory-Python/images/Concurrent.svg)

Multiple MATLAB instances run in parallel, each on a separate CPU/core path, and all connect to Python. This allows much faster experience collection and training than a single simulation stream.

## What This Repository Demonstrates

- A unified communication approach (TCP) that works across different simulation ecosystems.
- Real-time closed-loop integration of external Python logic with simulation timesteps.
- Scalable experimentation: from classical control logic to reinforcement learning.
- A practical pathway for researchers and engineers to prototype advanced controllers without being limited by built-in simulator controller blocks.

## What I Did

- Designed and implemented a cross-platform co-simulation concept centered on Python TCP control.
- Built and validated a MATLAB/Simulink + Python RL pipeline under this concept.
- Built and validated a PowerFactory + DLL + Python synchronous controller bridge under the same concept.
- Added concurrent MATLAB execution so training can be distributed over multiple simulation workers.
- Structured the project so each framework can be used independently while sharing the same systems-level philosophy.

## Detailed Framework Documentation

This README is intentionally high-level.

For implementation details, setup, configuration, and step-by-step usage:

- [Matlab-Python/README.md](Matlab-Python/README.md)
- [PowerFactory-Python/Readme.md](PowerFactory-Python/Readme.md)

## Citation

If you use the RL contribution, cite DOI 10.1109/KPEC58008.2023.10215460.

Metadata is available in Citation.cff.

## Author

Rasool Peykarporsan  
ORCID: <https://orcid.org/0009-0006-5237-3847>
