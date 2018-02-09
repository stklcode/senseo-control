View this project on [CADLAB.io](https://cadlab.io/node/863). 

# Senseo Control 2.0 

A completely rebuilt AVR based hard- and software for Senseo® HD782x coffee machine.
It allows custom setting of cup size and temperature and includes simple support for a second cup size (e.g. espresso) by pushing the buttons longer (2s).

The original project was documented in [this blog post](https://www.stklblog.de/blog/senseo-control-20) (German), including photos of the prototype.

## Functionality

The OEM functionality (coffee, rinsing, auto-off) has been reimplemented (indications adapted to RGB LED) with a few features on top. 

### Features

|         |  Input                           | Action                                               |
|:-------:|:--------------------------------:|:----------------------------------------------------:|
| ⬤ ⭗ ⬤ | push both coffee buttons         | rinsing cycle (pump cold water, until tank is empty) |
| ⬤ ⭗ ⭕ | push single coffee button        | 1/2 cups of coffee                                   |
| ⬤ ⭗ ⭕ | push single coffee button for 2s | 1/2 cups of espresso (shorter time, 2s pre-brewing)  |
| ⭕ ⬤ ⭕ | push power button                | start / shutdown at any time                         |
| ⭕ ⭗ ⭕ | 3 minutes idle                   | Auto-Off                                             |

#### Optional Features

* **Coffee wish** - if enabled (see below), the machine will memorize your coffee wish during heat-up and automatically 
start the coffee when finished, indicating through a violet LED (instead of green) 


### LED Signals

| Color  | Light                                      | Flashing                                                     |
|:------:|:------------------------------------------:|:------------------------------------------------------------:|
| red    | -                                          | <span style="color:red">◍</span> heating up                  |
| green  | <span style="color:green">⬤</span> ready  | <span style="color:green">◍</span> coffee running            |
| orange | -                                          | <span style="color:orange">◍</span> espresso running         |
| blue   | <span style="color:blue">⬤</span> rinsing | <span style="color:blue">◍</span> water empty                |
| violet | -                                          | <span style="color:violet">◍</span> heating up (coffee wish) |


## Platform

### Hardware

The hardware is based on an **ATtiny26** microcontroller with internal 8MHz RC oscillator.

Power supply is provided by a small transforer with a _78L05_ linear regulator. 
Pump and boiler are controlled by Triacs with isolated MOC30xx drivers.
The original sensors (NTC for temperature, Hall for water tank switch) are directly attached to the new board.

### Schematic and Layout

The prototype has been built in THT on a perfboard, hence the layout provided is compatible to 2.54mm (100mil) grid.
Might as well be used for a PCB. The board is designed to fit the HD7822 model.

Files provided under _hardware/_ are compatible with CadSoft/Autodesk EAGLE™ 6 and above.

### Firmware

The firmware is written in _C_ and comes with a _Makefile_ for use with _avr-gcc_ and _avrdude_.
There are configurations available for _STK500_, _AVR ISP mkII_ and _Pony-STK200_ which can be adapted to your setup.

## Customization

The code is designed to customize functions, timing, temperature and hardware pinning.
To to so, set the corresponding fields in the _main.h_ file:

| Flag                    | Default | Description                                  |
|-------------------------|---------|----------------------------------------------|
| `TIME_1_ESPRESSO`       | 15      | pump time 1 espresso (seconds)               |
| `TIME_2_ESPRESSO`       | 28      | pump time 2 espressos (seconds)              |
| `TIME_1_COFFEE`         | 26      | pump time 1 coffee (seconds)                 |
| `TIME_2_COFFEE`         | 52      | pump time 2 coffees (seconds)                |
| `OPERATING_TEMPERATURE` | 125     | water temperature (ADC value)                |
| `AUTO_OFF_THRESHOLD`    | 180     | Auto-Off time (seconds  after last action)   |
| `COFFEE_WISH`           | 0       | save coffee wish on heat-up (`1` to enable)  |

Pinout, button-thresholds and LED-configuration is also present in this file (should be self-explaining).


## Build Instructions
* Required tools: _avr-gcc_, _avr-objcopy_, _avrdude_ (for flashing only), _make_
* All sources are bundled in the `firmware` directory
* Check `Makefile.config` for the correct settings, especially tool and port for automated flashing.
* On first build you might want to set the correct fuse bits, so run `make fuses`
* Run `make compile info program` for compilation, details about binary, and flashing
* Check `make help` for all available commands

## Notes

The Triacs need heatsink.
The prototype uses approx. 75 x 30 x 1.5 mm (3 x 1.2 x 0.06 in) aluminum sheet, which is about the minimum recommended with a maximum temperature around 70°C (158°F).

<span style="background-color:lightyellow;padding:.5em;border-left:4px orange solid;display:block">**⚡ Danger - High Voltage ⚡**<br>
Working on a live coffee machine involves voltages of 120/230V AC **!** This voltage is applied to the heatsinks and causes hazard on contact.<br>
Please make sure you know what you are doing or consult an authorized professional.<br>
<br>
Cut the power before working on the device.
First tests can be done without main power (supply the µC with 5V from the programmer), Triacs can be tested with low voltage (e.g. 12V~).
On final test use an isolating transformer or at least a RCD.</span>


## License

The project is licensed under [GPL v3](https://www.gnu.org/licenses/gpl-3.0.de.html) license.

## Disclaimer

_"SENSEO"_ is a Trademark of _Koninklijke Philips N.V._ when used for coffee makers and a Trademark of _Sara Lee/De N.V._ when used for coffee pods.
No claim is made to the exclusive right to use _Senseo_ apart from the mark as shown.
