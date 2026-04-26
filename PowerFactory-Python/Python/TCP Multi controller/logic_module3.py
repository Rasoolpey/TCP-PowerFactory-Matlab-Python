# logic_module2.py

import math

def compute_outputs(inputs):
    """
    Custom control logic for PowerFactory communication.
    Expected:
        inputs = [y1, y2, y3, y4]
    Returns:
        [output1, output2]
    """
    if len(inputs) != 7:
        raise ValueError("Expected 4 input signals.")

    y1, y2, y3, y4, y5, y6, y7 = inputs

    # Example control logic
    u1r = math.sin(inputs[2]) 
    u2r = math.sin(inputs[1])      # e.g., sin(time)
    # u1i = math.sin(y4)       # e.g., sin(floor(time))

    return [u1r,u2r]
