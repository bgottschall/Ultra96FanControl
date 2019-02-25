# Ultra96 Fan PWM Control

Based on AndrewCapon's Ultra96 Fan PWM IP:
* https://github.com/AndrewCapon/Ultra96FanControl
* https://www.hackster.io/andycap/ultra96-fan-control-21fb8b

## fanctrl

```text
FanCTRL - Control Ultra96 Fan PWM

Read Fan State (default):
    -r, --read                 read state
    -w, --watch                continous read
    -u, --update               set update interval (ms)

Set Fan Paramters:
    -l, --low-temp <celsius>   set low temp
    -m, --max-temp <celsius>   set max temp
    -s, --smooth <smooth>      set smooth divisor
    -a, --auto                 automatic pwm control
    -f, --fixed <duty-cycle>   fixed pwm duty cycle

Miscellaneous:
    -h, --help                 this text
    -d, --debug                much more information
```

## Compile

```bash
make S_AXI_BASEADDR=0x80000000
```
