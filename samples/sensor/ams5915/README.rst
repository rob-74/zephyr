.. _bme280:

BME280 Humidity and Pressure Sensor
###################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`AMS 5915`_ sensor.

.. _AMS 5915:
   https://https://www.analog-micro.com/de/produkte/drucksensoren/board-mount-drucksensoren/ams5915/

The sample periodically reads temperature and pressure data from the
first available AMS5915 device discovered in the system. The sample checks the
sensor in polling mode (without interrupt trigger).

Building and Running
********************

The sample can be configured to support AMS5915 sensors connected via I2C. Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "anamicro,ams5915";``. See
:dtcompatible:`anamicro,ams5915` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `AMS5915 datasheet`_ at
page 7.

.. _AMS5915 datasheet:
   https://https://www.analog-micro.com/products/pressure-sensors/board-mount-pressure-sensors/ams5915/ams5915-datasheet.pdf

BME280 via Arduino I2C pins
===========================

If you wired the sensor to an I2C peripheral on an Arduino header, build and
flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ams5915
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay :zephyr_file:`samples/sensor/ams5915/arduino_i2c.overlay`
works on any board with a properly configured Arduino pin-compatible I2C
peripheral.

Board-specific overlays
=======================

If your board's devicetree does not have a BME280 node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`nucleo_g474re` using the
:zephyr_file:`samples/sensor/ams5915/boards/nucleo_g474re.overlay`
overlay provided with this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ams5915
   :goals: build flash
   :board: nucleo_g474re

Sample Output
=============

The sample prints output to the serial console. ams5915 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one AMS5915 sensor is connected to the standard Arduino I2C pins:

.. code-block:: none

   [00:00:00.379,760] <dbg> BME280.bme280_init: initializing "BME280_SPI" on bus "SPI_3"
   [00:00:00.379,821] <dbg> BME280.bme280_init: bad chip id 0xff
   [00:00:00.379,821] <dbg> BME280.bme280_init: initializing "BME280_I2C" on bus "I2C_0"
   [00:00:00.380,340] <dbg> BME280.bme280_init: ID OK
   [00:00:00.385,559] <dbg> BME280.bme280_init: BME280_I2C OK
   *** Booting Zephyr OS build zephyr-v2.4.0-2940-gbb732ada394f  ***
   Found device BME280_I2C, getting sensor data
   temp: 20.260000; press: 99.789019; humidity: 46.458984
   temp: 20.260000; press: 99.789480; humidity: 46.424804
   temp: 20.250000; press: 99.789246; humidity: 46.423828

Here is example output for the default application settings, assuming that two
different BME280 sensors are connected to the standard Arduino I2C and SPI pins:

.. code-block:: none

   [00:00:00.377,777] <dbg> BME280.bme280_init: initializing "BME280_SPI" on bus "SPI_3"
   [00:00:00.377,838] <dbg> BME280.bme280_init: ID OK
   [00:00:00.379,608] <dbg> BME280.bme280_init: BME280_SPI OK
   [00:00:00.379,638] <dbg> BME280.bme280_init: initializing "BME280_I2C" on bus "I2C_0"
   [00:00:00.380,126] <dbg> BME280.bme280_init: ID OK
   [00:00:00.385,345] <dbg> BME280.bme280_init: BME280_I2C OK
   *** Booting Zephyr OS build zephyr-v2.4.0-2940-gbb732ada394f  ***
   Found device BME280_I2C, getting sensor data
   temp: 20.150000; press: 99.857675; humidity: 46.447265
   temp: 20.150000; press: 99.859121; humidity: 46.458984
   temp: 20.150000; press: 99.859234; humidity: 46.469726

That the driver logs include a line saying ``BME280_I2C OK`` in both cases, but
``BME280_SPI OK`` is missing when that device is not connected.
