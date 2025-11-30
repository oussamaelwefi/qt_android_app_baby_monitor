<div align="center">
  <img src="./images/Android_robot.svg.png" alt="android logo" width="200"/>
  <img src="./images/Qt_logo_2016.svg.png" alt="qt logo" width="200"/>
  <img src="./images/1200px-ONNX_logo_main.png" alt="onnx logo" width="200"/>
</div>
<h1> Welcome to this Android app built with qt. </h1>
<h3>This app is for monitoring data in real time coming from an esp32-cam using bluetooth low energy. And predicts health risks using a machine learning model executed using onnx runtime built for android.</h3>
The gui is built using only c++. We have two interfaces (giuwindow and fff classes). <br>
The app is communicating with an esp32-cam using ble (bluetooth low energy) technology.<br>
The esp32-cam gets the sensors information from an esp8266, you can find the code for both boards here: <br>
The app is using a machine learning model to predict wether the baby is in good health or not.<br>
It is using the onnx runtime built for android, you can find it here : https://github.com/oussamaelwefi/onnx_runtime_for_android<br>
You can find the training notebook here : https://github.com/oussamaelwefi/esp32_ble_qt/tree/main<br>
This is how the app looks like : <br>
<div align="center">
  <img src="./images/584787959_1582411672783812_4624606882635581770_n.jpg" alt="the form" width="400"/>
  <img src="./images/588090087_1555286062455952_7189427724099501655_n.jpg" alt="status ok" width="400"/>
  <img src="./images/588569949_1039821498270697_8904180359565477016_n.jpg" alt="status at risk" width="400"/>
</div>
