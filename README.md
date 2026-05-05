# Gesture-Controlled-Robot
Collaborated with a partner to build a gesture-controlled robot using a 3D-printed wand with a 9-DoF IMU to capture and wirelessly transmit directional wrist motions to a PSoC-controlled car. Integrated ultrasonic proximity sensing with dynamic LED control for obstacle detection within ~10 cm. This meant to drive around based on the movements of a magic wand and our wand has an IMU that is connected to an Arduino Uno with an XBee Shield with an XBee that communicates to an XBee connected to the PSOC on our
 robot. Upon encountering various obstacles, our robot uses a sensor which will trigger our LED lights to flash red lights on and off to show that there is an obstacle in proximity. When this is not happening, the LED lights follow a dynamic rainbow pattern around our robot. 

 Refer to main.c file for main code! for PSoC Creator!
