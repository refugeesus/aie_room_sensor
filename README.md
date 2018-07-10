# aie_room_sensor
Presence, tvoc, co2 sensor build on Helium for AIE


## Setup

We are using Platform IO here so I would download it via brew on OSX or follow the instructions on their webpage for other systems. 

```bash
brew install platformio
```

After Platform IO is installed, navigate to the root directory of grideye and:

```bash
cd grideye
pio run
pio run -t upload -t monitor
```

`pio run` will build the application. The option `-t upload` will upload to the board. The option `-t monitor` will monitor the serial port for print messages from the app.
