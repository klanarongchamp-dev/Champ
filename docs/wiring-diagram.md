# Wiring Diagram

| Function | NodeMCU Pin | GPIO | Relay Input |
| --- | --- | --- | --- |
| Pump | D1 | GPIO5 | IN1 |
| Zone1 | D2 | GPIO4 | IN2 |
| Zone2 | D5 | GPIO14 | IN3 |
| Zone3 | D6 | GPIO12 | IN4 |

```text
NodeMCU 3V3  -> Relay VCC if 3.3V module, otherwise use isolated 5V supply
NodeMCU GND  -> Relay GND
D1/GPIO5     -> IN1 Pump
D2/GPIO4     -> IN2 Zone1
D5/GPIO14    -> IN3 Zone2
D6/GPIO12    -> IN4 Zone3
Relay COM    -> Pump/Valve supply line
Relay NO     -> Pump/Valve load input
```

Use opto-isolated relay boards and a flyback-protected DC supply for valves. Keep mains wiring physically separated and fused.
