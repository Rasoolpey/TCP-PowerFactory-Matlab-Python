# Real-Time TCP Co-Simulation Bridge for DIgSILENT PowerFactory

## Motivation

DIgSILENT PowerFactory does not natively support advanced or AI-based controllers. This project solves that by creating a synchronous, real-time data bridge between a PowerFactory simulation and any external Python script (or any other application) running on the same machine via TCP.

During a simulation, PowerFactory calls a function in this DLL, which sends measurement signals out to an external Python process, waits for a response, and returns the computed control outputs back into the simulation — all in the same timestep. This means you can implement any control logic, machine learning model, or optimization algorithm in Python and have it run as if it were a native PowerFactory controller.

---

## How It Works

```
PowerFactory Simulation
        |
        |  calls ExchangeViaTCP(y1, y2, ..., yN)
        v
   digexfun.dll
        |  packs N signals as raw bytes
        |  sends over TCP to Python server
        |  waits for M output values back
        v
   Python tcp_server.py
        |  receives inputs
        |  calls compute_outputs() in logic_module.py
        |  sends results back
        v
   digexfun.dll
        |  stores outputs in internal buffer
        |  simulation script reads each output with GetOutputIndex(i)
        v
PowerFactory uses the outputs as control signals
```

The exchange is **synchronous** — PowerFactory blocks until it gets a response, which keeps the simulation and the external controller in lock-step.

---

## Two Variants

This repository includes two variants depending on your use case.

---

### Variant 1 — Single Controller, Multiple Inputs/Outputs
**Folder:** `Python/TCP several IO/`
**Example project:** `PFModel/DLL Multi_input.pfd`

For when you have **one controller** that takes multiple measurement signals and returns multiple control outputs.

**DLL Functions:**

#### `ExchangeViaTCP(y1, y2, y3, y4)`
Sends 4 measurement signals to the Python server and receives 2 control outputs. The outputs are stored internally in a buffer.

| Argument | Description |
|---|---|
| `y1` – `y4` | Measurement signals from the simulation (e.g., voltage, current, power, time) |

> The number of inputs and outputs is configured in `userfun.cpp` via `NUM_INPUTS` and `NUM_OUTPUTS`.

#### `GetOutputIndex(i)`
Retrieves output `i` from the buffer filled by the last `ExchangeViaTCP` call.

| Argument | Description |
|---|---|
| `i` | Zero-based index of the output to retrieve (`0` = first output, `1` = second, etc.) |

**How to call it in a PowerFactory DSL script:**

```
! Send signals to the Python controller and receive outputs
ExchangeViaTCP(Vmeas, Imeas, Pmeas, time)

! Read each output individually
double u1, u2
u1 = GetOutputIndex(0)
u2 = GetOutputIndex(1)
```

**Python side — `tcp_server.py`:**

```python
NUM_INPUTS = 4    # must match NUM_INPUTS in userfun.cpp
NUM_OUTPUTS = 2   # must match NUM_OUTPUTS in userfun.cpp
```

Your control logic goes in `logic_module.py`, in the `compute_outputs(inputs)` function:

```python
def compute_outputs(inputs):
    y1, y2, y3, y4 = inputs
    u1 = ...  # your control algorithm
    u2 = ...
    return [u1, u2]
```

**To run:**
```
python tcp_server.py
```
Start the Python server **before** launching the simulation in PowerFactory.

---

### Variant 2 — Multiple Independent Controllers
**Folder:** `Python/TCP Multi controller/`
**Example project:** `PFModel/DLL Multi_Control.pfd`

For when you have **multiple components** (e.g., multiple inverters or generators), each with its own independent controller running on a separate port.

Each component gets its own DLL function and its own Python server process, so they run independently in parallel.

**Default configuration (3 components):**

| Function | Port | Inputs | Outputs | Logic module |
|---|---|---|---|---|
| `ExchangeViaTCP0` | 9101 | 7 | 2 | `logic_module1.py` |
| `ExchangeViaTCP1` | 9102 | 7 | 2 | `logic_module2.py` |
| `ExchangeViaTCP2` | 9103 | 7 | 2 | `logic_module3.py` |

#### `ExchangeViaTCP0(y1, ..., y7)` / `ExchangeViaTCP1(...)` / `ExchangeViaTCP2(...)`
Each function sends signals to its assigned Python server and stores the received outputs in the shared buffer.

#### `GetOutputIndex(i)`
Same as Variant 1 — retrieves output `i` from the buffer of the most recently called component.

**How to call it in a PowerFactory DSL script:**

```
! Controller for component 0
ExchangeViaTCP0(V1, I1, P1, Q1, Vref, Iref, time)
double u1a, u1b
u1a = GetOutputIndex(0)
u1b = GetOutputIndex(1)

! Controller for component 1
ExchangeViaTCP1(V2, I2, P2, Q2, Vref, Iref, time)
double u2a, u2b
u2a = GetOutputIndex(0)
u2b = GetOutputIndex(1)
```

**Python side — `tcp_server.py`:**

The multi-controller server spawns one process per component automatically:

```python
COMPONENTS = [
    {"port": 9101, "num_inputs": 7, "num_outputs": 2, "module": "logic_module1"},
    {"port": 9102, "num_inputs": 7, "num_outputs": 2, "module": "logic_module2"},
    {"port": 9103, "num_inputs": 7, "num_outputs": 2, "module": "logic_module3"},
]
```

Each `logic_moduleN.py` file has a `compute_outputs(inputs)` function where you write the logic for that component independently.

**To run:**
```
python tcp_server.py
```
This starts all component servers in parallel. Start it **before** the simulation.

---

## PowerFactory Setup

See the reference screenshots:

- `images/FunctionDefinition_MultiOutput.png` — how to define the external function in PowerFactory
- `images/data_exchange.png` — the data flow between PowerFactory and the DLL

General steps:
1. Place `digexfun.dll` in your PowerFactory external functions folder.
2. In PowerFactory, create an **External Function** object pointing to the DLL.
3. Call `ExchangeViaTCP(...)` and `GetOutputIndex(i)` from your DSL simulation script or composite model.

---

## How to Modify

### Change the number of inputs or outputs (Variant 1)
Edit the two defines at the top of `Python/TCP several IO/DLL_TCPConnection/userfun.cpp`:

```cpp
#define NUM_INPUTS  4   // number of signals sent to Python
#define NUM_OUTPUTS 2   // number of values received from Python
```

Update `NUM_INPUTS` and `NUM_OUTPUTS` in `tcp_server.py` to match.

### Change the control algorithm
Edit `compute_outputs(inputs)` in `logic_module.py` (Variant 1) or `logic_moduleN.py` (Variant 2). No DLL recompilation needed.

### Add or remove components (Variant 2)
In `Python/TCP Multi controller/TCP_CPP_Code/userfun.cpp`, update the `COMPONENTS` array and `NUM_COMPONENTS`:

```cpp
#define NUM_COMPONENTS 3

const ControlTarget COMPONENTS[NUM_COMPONENTS] = {
    {9101, 7, 2},  // Component 0: port, num_inputs, num_outputs
    {9102, 7, 2},  // Component 1
    {9103, 7, 2},  // Component 2
};
```

Add the matching entry in `tcp_server.py`'s `COMPONENTS` list and create a `logic_moduleN.py` for it.

### Rebuild after C++ changes
Open the `.sln` file in Visual Studio, build as **Release x64**, and replace `digexfun.dll`.
