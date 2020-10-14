# Sunblind Controller Unit

A controller unit designed to be used in a fictional product that regulates a sunblind. This was made as a project for a university course.

## Features

- Autonomously rolls the sunblind in or out using ambient temperature or light.
- Uses a sonar sensor to check wether the sunblind is rolled in or not.
- Sends information about the unit over serial connection.
- Has a communication API to control the unit over serial connection.
    - Can overwrite the sunblind state.
    - Can configure boundary values for the sensors.
