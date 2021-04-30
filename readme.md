<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://github.com/LiveSparks/PowerMateWinBridge">
    <img src="PowerMateWinBridge/dial_knob.ico" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">PowerMate Bluetooth Windows Bridge</h3>

  <p align="center">
    Maps the Griffin PowerMate Bluetooth's Inputs to extra function keys on Windows.
  </p>
</p>

Most of the code is stolen from [s7726/PowerMate](https://github.com/s7726/PowerMate). Project was written in Visual Studio 2019. Allows you to map the PowerMate's input like normal keys in any application. Recommended to be used with [AutoHotKey](https://www.autohotkey.com/).

## Usage

1. Download the PowerMateWinBridge.exe binary from the [releases](https://github.com/LiveSparks/PowerMateWinBridge/releases) page.
2. Make sure that Bluetooth is ON and the PowerMate is turned on.
3. Run the executable and monitor the ouptut for a successful connection.
4. Check if turning the knob on the PowerMate produces a output on the console.
5. If the connection fails, try pairing to the device from windows settings before running the program
6. The default button mapping is as follows:
```
POWERMATE           ->  KEY
----------------------------
CLICK               ->  F13
LEFT TURN           ->  F14
RIGHT TURN          ->  F15
PRESS + LEFT TURN   ->  F16
PRESS + RIGHT TURN  ->  F17
LONG PRESS          ->  F18
```
7. You can either bind the PowerMate's input in any software as you would any other key. Or,
8. Use AutoHotKey to make custom global keybinds. An example AHK script:
```ahk
F14::Volume_Down
F15::Volume_Up
```

## Contact

Ravi Kharb – [@LiveSparksYT](https://twitter.com/LiveSparksYT) – livesparking@gmail.com

## Contributing

1. Fork it (https://github.com/LiveSparks/PowerMateWinBridge)
2. Create your feature branch (`git checkout -b feature/fooBar`)
3. Commit your changes (`git commit -am 'Add some fooBar'`)
4. Push to the branch (`git push origin feature/fooBar`)
5. Create a new Pull Request

<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements
* [s7726/PowerMate](https://github.com/s7726/PowerMate)