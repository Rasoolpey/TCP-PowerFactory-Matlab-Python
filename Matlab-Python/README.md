# Matlab-Python RL: Parallel DDPG Control of a Buck Converter

A research framework that trains a Deep Deterministic Policy Gradient (DDPG) agent in **Python/PyTorch** to regulate the output voltage of a **Buck DC-DC converter** simulated in **MATLAB/Simulink**. Multiple Simulink instances run in parallel and stream state to the agent over TCP, dramatically accelerating experience collection.

---

## Overview

The agent learns to choose the **duty cycle** `u ∈ [0, 1]` of a Buck converter so that the output voltage `Vo` tracks a reference `Vref = 7.5 V` quickly and with minimal overshoot.

- **Environment** – Simulink model [Buck_Converter.slx](Buck_Converter.slx) with TCP send/receive blocks streaming `(V, IL, t)` to Python and receiving the duty cycle back.
- **Agent** – DDPG with separate Actor / Critic networks, target networks with soft updates (`τ = 0.001`), and an experience replay buffer.
- **Parallelism** – `N` independent Simulink simulations run on separate loopback IPs (`127.0.0.100` … `127.0.0.107`). Each simulation feeds its own thread; a `threading.Barrier` synchronizes them so the shared network is trained on batched experience between control steps.

## Repository Layout

```
.
├── Buck_RL.py                    # Main training driver
├── Buck_Converter.slx            # Simulink plant model
├── run_simulations.m             # MATLAB launcher (parsim across IPs)
├── Citation.cff                  # Citation metadata
├── modules/
│   ├── config_module.py          # Hyperparameters, IP list, networks, optimizers
│   ├── networks_module.py        # Actor and Critic MLPs
│   ├── rl_training_utils.py      # ReplayBuffer, DDPG update, action selection
│   ├── simulation_module.py      # Episode rollout + MATLAB engine entrypoint
│   ├── tcp_module.py             # TCP socket bridge to Simulink
│   ├── decision_evaluation_module.py  # Composite reward + DoneChecker
│   └── plotting_module.py        # Per-episode result plots
├── plots/                        # Saved per-episode plots (svg)
└── warehouse/                    # Misc. project assets
```

## How It Works

### Control loop (per episode, per worker)

1. Python opens a TCP socket on a unique loopback IP and waits for a Simulink instance to connect.
2. MATLAB launches `N` parallel Simulink runs via `parsim`, each pointed at one of those IPs.
3. At every control step, the worker:
   - sends the current duty cycle `u` to Simulink,
   - receives `(V, IL, t)` back,
   - computes the composite reward,
   - holds the action for `action_duration` simulation steps before re-querying the actor.
4. A `Barrier` blocks all workers until they reach the synchronization point; its `action` callback runs `train_network`, draws a batch from the shared replay buffer, performs one DDPG update, and then releases the workers for the next action.

### Reward

`composite_reward` combines four terms (see [decision_evaluation_module.py](modules/decision_evaluation_module.py)):

| Term | Meaning | Weight |
|------|---------|--------|
| Stability | `−(V − Vref)²` | 1.0 |
| Efficiency | `−0.01·(u − uₚᵣₑᵥ)²` (penalize chattering) | 0.001 |
| Convergence | improvement in `|V − Vref|` vs. previous step | 0.5 |
| Time | `−t / runtime` (encourage fast settling) | 1.5 |

`DoneChecker` declares an episode complete once `V` stays inside ±3% of `Vref` for 0.5 s.

### Networks

- **Actor**: `state_dim → 64 → 128 → action_dim` with ReLU hidden, sigmoid output scaled by `max_action = 1.0`.
- **Critic**: `(state ⊕ action) → 64 → 128 → 1` with ReLU hidden.
- Optimized with Adam, learning rate `1e-3`, discount `γ = 0.99`.

### Replay buffer

Capacity 100,000 transitions. Sampling is biased toward non-zero actions (`non_zero_ratio = 0.8`) to counter the early-training tendency to sit at `u = 0`.

## Requirements

**Python** (3.9+ recommended)

```text
torch
numpy
scipy
matplotlib
matlabengine          # MATLAB Engine API for Python
```

**MATLAB / Simulink** with:

- Simulink
- Simscape Electrical (for the Buck plant)
- Parallel Computing Toolbox (for `parsim`)
- The Instrument Control Toolbox or equivalent TCP/IP send/receive blocks

The MATLAB Engine API for Python must be installed against your MATLAB release — see [MathWorks: Install MATLAB Engine API for Python](https://www.mathworks.com/help/matlab/matlab_external/install-the-matlab-engine-for-python.html).

## Configuration

Edit [modules/config_module.py](modules/config_module.py) to change:

- `num_episodes`, `runtime`, `action_duration`
- `Vref`, `Vinit`, `Iinit`
- DDPG hyperparameters (`DISCOUNT`, `LEARNING_RATE`, `tau`, `epsilon*`)
- `training_batch_size`
- `ips` — one loopback IP per parallel Simulink worker

> The MATLAB working directory inside [simulation_module.py](modules/simulation_module.py) (`eng.cd(...)`) is currently hard-coded. Update it to point at the directory containing `run_simulations.m` on your machine.

## Running

```bash
python Buck_RL.py
```

This will:

1. Spawn a thread that boots the MATLAB engine and launches `parsim` across all configured IPs.
2. Open one TCP listener per IP and wait for the Simulink workers to connect.
3. Run `episode_per_ip = num_episodes // len(ips)` synchronized batches.
4. After every batch, plot the best-performing episode to `plots/episode_number_<i>.svg`.

## Output

Each batch saves a two-panel SVG plot of the best episode:

- **Top** – output voltage `Vo` (orange) overlaid with duty cycle (blue).
- **Bottom** – per-step composite reward.

## Citation

See [Citation.cff](Citation.cff) for full metadata.

## Author

**Rasool Peykarporsan** — PhD Student
ORCID: [0009-0006-5237-3847](https://orcid.org/0009-0006-5237-3847)
Repository: [github.com/Rasoolpey/Matlab-python-RL](https://github.com/Rasoolpey/Matlab-python-RL)
