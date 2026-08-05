<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="7.3.0">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="no" active="no"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="no" active="no"/>
<layer number="17" name="Pads" color="2" fill="1" visible="no" active="no"/>
<layer number="18" name="Vias" color="2" fill="1" visible="no" active="no"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="no" active="no"/>
<layer number="20" name="Dimension" color="15" fill="1" visible="no" active="no"/>
<layer number="21" name="tPlace" color="14" fill="1" visible="no" active="no"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="25" name="tNames" color="7" fill="1" visible="no" active="no"/>
<layer number="26" name="bNames" color="7" fill="1" visible="no" active="no"/>
<layer number="27" name="tValues" color="7" fill="1" visible="no" active="no"/>
<layer number="28" name="bValues" color="7" fill="1" visible="no" active="no"/>
<layer number="90" name="Modules" color="5" fill="1" visible="yes" active="yes"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="smart-gateway">
<description>&lt;b&gt;Smart Gateway edge tier&lt;/b&gt;&lt;p&gt;
Schematic-only symbols for the access-control bench node. Devices carry no
package: this sheet documents interconnect, not a board layout.</description>
<packages>
</packages>
<symbols>
<symbol name="ARDUINO_UNO">
<wire x1="-25.4" y1="-33.02" x2="25.4" y2="-33.02" width="0.254" layer="94"/>
<wire x1="25.4" y1="-33.02" x2="25.4" y2="33.02" width="0.254" layer="94"/>
<wire x1="25.4" y1="33.02" x2="-25.4" y2="33.02" width="0.254" layer="94"/>
<wire x1="-25.4" y1="33.02" x2="-25.4" y2="-33.02" width="0.254" layer="94"/>
<text x="-17.78" y="2.54" size="3.81" layer="94">ARDUINO</text>
<text x="-12.7" y="-3.81" size="2.54" layer="94">UNO R3</text>
<text x="-17.78" y="-15.24" size="1.778" layer="94">Policy Enforcement Point</text>
<text x="-25.4" y="34.29" size="1.778" layer="95">&gt;NAME</text>
<pin name="D13" x="-33.02" y="30.48" visible="pin" length="middle"/>
<pin name="D12" x="-33.02" y="27.94" visible="pin" length="middle"/>
<pin name="D11" x="-33.02" y="25.4" visible="pin" length="middle"/>
<pin name="D10" x="-33.02" y="22.86" visible="pin" length="middle"/>
<pin name="D9" x="-33.02" y="20.32" visible="pin" length="middle"/>
<pin name="D8" x="-33.02" y="17.78" visible="pin" length="middle"/>
<pin name="D7" x="-33.02" y="15.24" visible="pin" length="middle"/>
<pin name="D6" x="-33.02" y="12.7" visible="pin" length="middle"/>
<pin name="D5" x="-33.02" y="10.16" visible="pin" length="middle"/>
<pin name="D4" x="-33.02" y="7.62" visible="pin" length="middle"/>
<pin name="D3" x="-33.02" y="5.08" visible="pin" length="middle"/>
<pin name="D2" x="-33.02" y="2.54" visible="pin" length="middle"/>
<pin name="D1" x="-33.02" y="0" visible="pin" length="middle"/>
<pin name="D0" x="-33.02" y="-2.54" visible="pin" length="middle"/>
<pin name="AREF" x="-33.02" y="-7.62" visible="pin" length="middle"/>
<pin name="RESET" x="-33.02" y="-10.16" visible="pin" length="middle"/>
<pin name="A0" x="33.02" y="30.48" visible="pin" length="middle" rot="R180"/>
<pin name="A1" x="33.02" y="27.94" visible="pin" length="middle" rot="R180"/>
<pin name="A2" x="33.02" y="25.4" visible="pin" length="middle" rot="R180"/>
<pin name="A3" x="33.02" y="22.86" visible="pin" length="middle" rot="R180"/>
<pin name="A4/SDA" x="33.02" y="20.32" visible="pin" length="middle" rot="R180"/>
<pin name="A5/SCL" x="33.02" y="17.78" visible="pin" length="middle" rot="R180"/>
<pin name="5V" x="33.02" y="10.16" visible="pin" length="middle" direction="pwr" rot="R180"/>
<pin name="3V3" x="33.02" y="7.62" visible="pin" length="middle" direction="pwr" rot="R180"/>
<pin name="VIN" x="33.02" y="5.08" visible="pin" length="middle" direction="pwr" rot="R180"/>
<pin name="GND" x="33.02" y="2.54" visible="pin" length="middle" direction="pwr" rot="R180"/>
<pin name="GND1" x="33.02" y="0" visible="pin" length="middle" direction="pwr" rot="R180"/>
</symbol>
<symbol name="MFRC522">
<wire x1="-12.7" y1="-6.35" x2="12.7" y2="-6.35" width="0.254" layer="94"/>
<wire x1="12.7" y1="-6.35" x2="12.7" y2="6.35" width="0.254" layer="94"/>
<wire x1="12.7" y1="6.35" x2="-12.7" y2="6.35" width="0.254" layer="94"/>
<wire x1="-12.7" y1="6.35" x2="-12.7" y2="-6.35" width="0.254" layer="94"/>
<text x="-6.35" y="1.27" size="2.032" layer="94">MFRC522</text>
<text x="-6.35" y="-3.81" size="1.27" layer="94">13.56 MHz</text>
<text x="-12.7" y="7.62" size="1.778" layer="95">&gt;NAME</text>
<pin name="SCK" x="20.32" y="5.08" visible="pin" length="middle" rot="R180"/>
<pin name="MISO" x="20.32" y="2.54" visible="pin" length="middle" rot="R180"/>
<pin name="MOSI" x="20.32" y="0" visible="pin" length="middle" rot="R180"/>
<pin name="SDA" x="20.32" y="-2.54" visible="pin" length="middle" rot="R180"/>
<pin name="RST" x="20.32" y="-5.08" visible="pin" length="middle" rot="R180"/>
<pin name="3V3" x="-20.32" y="5.08" visible="pin" length="middle" direction="pwr"/>
<pin name="GND" x="-20.32" y="-5.08" visible="pin" length="middle" direction="pwr"/>
</symbol>
<symbol name="LCD1602_I2C">
<wire x1="-15.24" y1="-10.16" x2="15.24" y2="-10.16" width="0.254" layer="94"/>
<wire x1="15.24" y1="-10.16" x2="15.24" y2="10.16" width="0.254" layer="94"/>
<wire x1="15.24" y1="10.16" x2="-15.24" y2="10.16" width="0.254" layer="94"/>
<wire x1="-15.24" y1="10.16" x2="-15.24" y2="-10.16" width="0.254" layer="94"/>
<text x="-2.54" y="2.54" size="2.54" layer="94">LCD 1602</text>
<text x="-2.54" y="-2.54" size="1.778" layer="94">PCF8574 0x27</text>
<text x="-15.24" y="11.43" size="1.778" layer="95">&gt;NAME</text>
<pin name="SDA" x="-22.86" y="5.08" visible="pin" length="middle"/>
<pin name="SCL" x="-22.86" y="2.54" visible="pin" length="middle"/>
<pin name="VCC" x="-22.86" y="-2.54" visible="pin" length="middle" direction="pwr"/>
<pin name="GND" x="-22.86" y="-5.08" visible="pin" length="middle" direction="pwr"/>
</symbol>
<symbol name="KEYPAD4X4">
<wire x1="-12.7" y1="-17.78" x2="12.7" y2="-17.78" width="0.254" layer="94"/>
<wire x1="12.7" y1="-17.78" x2="12.7" y2="17.78" width="0.254" layer="94"/>
<wire x1="12.7" y1="17.78" x2="-12.7" y2="17.78" width="0.254" layer="94"/>
<wire x1="-12.7" y1="17.78" x2="-12.7" y2="-17.78" width="0.254" layer="94"/>
<text x="-2.54" y="2.54" size="2.54" layer="94">KEYPAD</text>
<text x="-2.54" y="-2.54" size="2.54" layer="94">4 x 4</text>
<text x="-6.35" y="-12.7" size="1.778" layer="94">col D unscanned</text>
<text x="-12.7" y="19.05" size="1.778" layer="95">&gt;NAME</text>
<pin name="R1" x="-20.32" y="15.24" visible="pin" length="middle"/>
<pin name="R2" x="-20.32" y="12.7" visible="pin" length="middle"/>
<pin name="R3" x="-20.32" y="10.16" visible="pin" length="middle"/>
<pin name="R4" x="-20.32" y="7.62" visible="pin" length="middle"/>
<pin name="C1" x="-20.32" y="2.54" visible="pin" length="middle"/>
<pin name="C2" x="-20.32" y="0" visible="pin" length="middle"/>
<pin name="C3" x="-20.32" y="-2.54" visible="pin" length="middle"/>
<pin name="C4" x="-20.32" y="-5.08" visible="pin" length="middle" direction="nc"/>
</symbol>
<symbol name="SERVO">
<wire x1="-12.7" y1="-7.62" x2="12.7" y2="-7.62" width="0.254" layer="94"/>
<wire x1="12.7" y1="-7.62" x2="12.7" y2="7.62" width="0.254" layer="94"/>
<wire x1="12.7" y1="7.62" x2="-12.7" y2="7.62" width="0.254" layer="94"/>
<wire x1="-12.7" y1="7.62" x2="-12.7" y2="-7.62" width="0.254" layer="94"/>
<text x="0" y="1.27" size="2.54" layer="94">SG90</text>
<text x="0" y="-3.81" size="1.778" layer="94">latch</text>
<text x="-12.7" y="8.89" size="1.778" layer="95">&gt;NAME</text>
<pin name="SIG" x="-20.32" y="2.54" visible="pin" length="middle"/>
<pin name="VCC" x="-20.32" y="0" visible="pin" length="middle" direction="pwr"/>
<pin name="GND" x="-20.32" y="-2.54" visible="pin" length="middle" direction="pwr"/>
</symbol>
<symbol name="ESP32CAM">
<wire x1="-15.24" y1="-15.24" x2="15.24" y2="-15.24" width="0.254" layer="94"/>
<wire x1="15.24" y1="-15.24" x2="15.24" y2="15.24" width="0.254" layer="94"/>
<wire x1="15.24" y1="15.24" x2="-15.24" y2="15.24" width="0.254" layer="94"/>
<wire x1="-15.24" y1="15.24" x2="-15.24" y2="-15.24" width="0.254" layer="94"/>
<text x="-2.54" y="10.16" size="2.54" layer="94">ESP32-CAM</text>
<text x="-2.54" y="5.08" size="1.778" layer="94">MJPEG2SD</text>
<text x="-2.54" y="1.27" size="1.778" layer="94">recorder</text>
<text x="-2.54" y="-2.54" size="1.778" layer="94">AI-Thinker</text>
<text x="-2.54" y="-6.35" size="1.778" layer="94">OV2640</text>
<text x="-15.24" y="16.51" size="1.778" layer="95">&gt;NAME</text>
<pin name="G13" x="-22.86" y="10.16" visible="pin" length="middle"/>
<pin name="G12" x="-22.86" y="7.62" visible="pin" length="middle"/>
<pin name="G16" x="-22.86" y="5.08" visible="pin" length="middle"/>
<pin name="5V" x="-22.86" y="-2.54" visible="pin" length="middle" direction="pwr"/>
<pin name="GND" x="-22.86" y="-5.08" visible="pin" length="middle" direction="pwr"/>
</symbol>
<symbol name="LVLSHIFT">
<wire x1="-12.7" y1="-10.16" x2="12.7" y2="-10.16" width="0.254" layer="94"/>
<wire x1="12.7" y1="-10.16" x2="12.7" y2="10.16" width="0.254" layer="94"/>
<wire x1="12.7" y1="10.16" x2="-12.7" y2="10.16" width="0.254" layer="94"/>
<wire x1="-12.7" y1="10.16" x2="-12.7" y2="-10.16" width="0.254" layer="94"/>
<wire x1="0" y1="10.16" x2="0" y2="-10.16" width="0.1524" layer="94" style="shortdash"/>
<text x="-10.16" y="7.62" size="1.27" layer="94">5V</text>
<text x="6.35" y="7.62" size="1.27" layer="94">3V3</text>
<text x="-8.89" y="0" size="2.032" layer="94">BSS138</text>
<text x="-12.7" y="11.43" size="1.778" layer="95">&gt;NAME</text>
<pin name="HV" x="-20.32" y="5.08" visible="pin" length="middle"/>
<pin name="HV1" x="-20.32" y="2.54" visible="pin" length="middle"/>
<pin name="HVCC" x="-20.32" y="-2.54" visible="pin" length="middle" direction="pwr"/>
<pin name="GND" x="-20.32" y="-7.62" visible="pin" length="middle" direction="pwr"/>
<pin name="LV" x="20.32" y="5.08" visible="pin" length="middle" rot="R180"/>
<pin name="LV1" x="20.32" y="2.54" visible="pin" length="middle" rot="R180"/>
<pin name="LVCC" x="20.32" y="-2.54" visible="pin" length="middle" direction="pwr" rot="R180"/>
</symbol>
<symbol name="RESISTOR">
<wire x1="-2.54" y1="0" x2="-2.159" y2="1.016" width="0.1524" layer="94"/>
<wire x1="-2.159" y1="1.016" x2="-1.524" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="-1.524" y1="-1.016" x2="-0.889" y2="1.016" width="0.1524" layer="94"/>
<wire x1="-0.889" y1="1.016" x2="-0.254" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="-0.254" y1="-1.016" x2="0.381" y2="1.016" width="0.1524" layer="94"/>
<wire x1="0.381" y1="1.016" x2="1.016" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="1.016" y1="-1.016" x2="1.651" y2="1.016" width="0.1524" layer="94"/>
<wire x1="1.651" y1="1.016" x2="2.286" y2="-1.016" width="0.1524" layer="94"/>
<wire x1="2.286" y1="-1.016" x2="2.54" y2="0" width="0.1524" layer="94"/>
<text x="-3.81" y="1.4986" size="1.778" layer="95">&gt;NAME</text>
<text x="-3.81" y="-3.302" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas" swaplevel="1"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" swaplevel="1" rot="R180"/>
</symbol>
<symbol name="LED">
<wire x1="-1.27" y1="2.54" x2="-1.27" y2="-2.54" width="0.254" layer="94"/>
<wire x1="-1.27" y1="0" x2="-3.81" y2="0" width="0.254" layer="94"/>
<wire x1="1.27" y1="2.54" x2="1.27" y2="-2.54" width="0.254" layer="94"/>
<wire x1="1.27" y1="0" x2="3.81" y2="0" width="0.254" layer="94"/>
<wire x1="-1.27" y1="0" x2="1.27" y2="2.54" width="0.254" layer="94"/>
<wire x1="1.27" y1="2.54" x2="1.27" y2="-2.54" width="0.254" layer="94"/>
<wire x1="1.27" y1="-2.54" x2="-1.27" y2="0" width="0.254" layer="94"/>
<wire x1="1.778" y1="3.048" x2="3.048" y2="4.318" width="0.1524" layer="94"/>
<wire x1="0.508" y1="3.556" x2="1.778" y2="4.826" width="0.1524" layer="94"/>
<text x="-3.81" y="5.08" size="1.778" layer="95">&gt;NAME</text>
<text x="-3.81" y="-6.35" size="1.778" layer="96">&gt;VALUE</text>
<pin name="A" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="C" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="BUZZER">
<wire x1="-2.54" y1="3.81" x2="2.54" y2="3.81" width="0.254" layer="94"/>
<wire x1="2.54" y1="3.81" x2="2.54" y2="-3.81" width="0.254" layer="94"/>
<wire x1="2.54" y1="-3.81" x2="-2.54" y2="-3.81" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-3.81" x2="-2.54" y2="3.81" width="0.254" layer="94"/>
<circle x="0" y="0" radius="1.27" width="0.1524" layer="94"/>
<text x="-3.81" y="5.08" size="1.778" layer="95">&gt;NAME</text>
<text x="-3.81" y="-7.62" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="VCC5">
<wire x1="0.762" y1="1.27" x2="0" y2="2.54" width="0.254" layer="94"/>
<wire x1="0" y1="2.54" x2="-0.762" y2="1.27" width="0.254" layer="94"/>
<text x="-2.54" y="3.556" size="1.778" layer="96">+5V</text>
<pin name="VCC" x="0" y="0" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
<symbol name="V3V3">
<wire x1="0.762" y1="1.27" x2="0" y2="2.54" width="0.254" layer="94"/>
<wire x1="0" y1="2.54" x2="-0.762" y2="1.27" width="0.254" layer="94"/>
<text x="-3.048" y="3.556" size="1.778" layer="96">+3V3</text>
<pin name="V3V3" x="0" y="0" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
<symbol name="DGND">
<wire x1="-1.905" y1="0" x2="1.905" y2="0" width="0.254" layer="94"/>
<text x="-2.54" y="-2.54" size="1.778" layer="96">GND</text>
<pin name="GND" x="0" y="2.54" visible="off" length="short" direction="sup" rot="R270"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="ARDUINO_UNO" prefix="U">
<description>Arduino UNO R3 - enforcement node (schematic only, no package)</description>
<gates>
<gate name="G$1" symbol="ARDUINO_UNO" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="MFRC522" prefix="U">
<description>RC522 13.56 MHz RFID reader module</description>
<gates>
<gate name="G$1" symbol="MFRC522" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="LCD1602_I2C" prefix="U">
<description>1602 character LCD with PCF8574 I2C backpack</description>
<gates>
<gate name="G$1" symbol="LCD1602_I2C" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="KEYPAD4X4" prefix="U">
<description>4x4 matrix membrane keypad</description>
<gates>
<gate name="G$1" symbol="KEYPAD4X4" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="SERVO" prefix="U">
<description>SG90 micro servo - latch actuator</description>
<gates>
<gate name="G$1" symbol="SERVO" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="ESP32CAM" prefix="U">
<description>ESP32-CAM movement recorder running ESP32-CAM_MJPEG2SD</description>
<gates>
<gate name="G$1" symbol="ESP32CAM" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="LVLSHIFT" prefix="U">
<description>BSS138 bidirectional level shifter, 5 V to 3.3 V</description>
<gates>
<gate name="G$1" symbol="LVLSHIFT" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="RESISTOR" prefix="R" uservalue="yes">
<gates>
<gate name="G$1" symbol="RESISTOR" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="LED" prefix="LED" uservalue="yes">
<gates>
<gate name="G$1" symbol="LED" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BUZZER" prefix="SG">
<gates>
<gate name="G$1" symbol="BUZZER" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="VCC5" prefix="SUPPLY">
<gates>
<gate name="G$1" symbol="VCC5" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="V3V3" prefix="SUPPLY">
<gates>
<gate name="G$1" symbol="V3V3" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="GND" prefix="GND">
<gates>
<gate name="1" symbol="DGND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="U1" library="smart-gateway" deviceset="ARDUINO_UNO" device=""/>
<part name="U2" library="smart-gateway" deviceset="MFRC522" device=""/>
<part name="U3" library="smart-gateway" deviceset="LCD1602_I2C" device=""/>
<part name="U4" library="smart-gateway" deviceset="KEYPAD4X4" device=""/>
<part name="U5" library="smart-gateway" deviceset="SERVO" device=""/>
<part name="U6" library="smart-gateway" deviceset="ESP32CAM" device=""/>
<part name="U7" library="smart-gateway" deviceset="LVLSHIFT" device=""/>
<part name="R1" library="smart-gateway" deviceset="RESISTOR" device="" value="220"/>
<part name="R2" library="smart-gateway" deviceset="RESISTOR" device="" value="220"/>
<part name="LED1" library="smart-gateway" deviceset="LED" device="" value="RED"/>
<part name="LED2" library="smart-gateway" deviceset="LED" device="" value="GRN"/>
<part name="SG1" library="smart-gateway" deviceset="BUZZER" device=""/>
<part name="SUPPLY1" library="smart-gateway" deviceset="VCC5" device=""/>
<part name="SUPPLY2" library="smart-gateway" deviceset="VCC5" device=""/>
<part name="SUPPLY3" library="smart-gateway" deviceset="VCC5" device=""/>
<part name="SUPPLY4" library="smart-gateway" deviceset="VCC5" device=""/>
<part name="SUPPLY5" library="smart-gateway" deviceset="VCC5" device=""/>
<part name="SUPPLY6" library="smart-gateway" deviceset="V3V3" device=""/>
<part name="SUPPLY7" library="smart-gateway" deviceset="V3V3" device=""/>
<part name="SUPPLY8" library="smart-gateway" deviceset="V3V3" device=""/>
<part name="GND1" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND2" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND3" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND4" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND5" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND6" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND7" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND8" library="smart-gateway" deviceset="GND" device=""/>
<part name="GND9" library="smart-gateway" deviceset="GND" device=""/>
</parts>
<sheets>
<sheet>
<plain>
</plain>
<instances>
<instance part="U1" gate="G$1" x="114.3" y="88.9"/>
<instance part="U2" gate="G$1" x="35.56" y="114.3"/>
<instance part="U3" gate="G$1" x="200.66" y="104.14"/>
<instance part="U4" gate="G$1" x="200.66" y="149.86"/>
<instance part="U5" gate="G$1" x="200.66" y="81.28"/>
<instance part="U6" gate="G$1" x="208.28" y="33.02"/>
<instance part="U7" gate="G$1" x="172.72" y="66.04"/>
<instance part="R1" gate="G$1" x="25.4" y="76.2"/>
<instance part="R2" gate="G$1" x="25.4" y="63.5"/>
<instance part="LED1" gate="G$1" x="43.18" y="76.2"/>
<instance part="LED2" gate="G$1" x="43.18" y="63.5"/>
<instance part="SG1" gate="G$1" x="27.94" y="50.8"/>
<instance part="SUPPLY1" gate="G$1" x="152.4" y="99.06"/>
<instance part="SUPPLY2" gate="G$1" x="170.18" y="101.6"/>
<instance part="SUPPLY3" gate="G$1" x="172.72" y="81.28"/>
<instance part="SUPPLY4" gate="G$1" x="177.8" y="30.48"/>
<instance part="SUPPLY5" gate="G$1" x="147.32" y="63.5"/>
<instance part="SUPPLY6" gate="G$1" x="7.62" y="119.38"/>
<instance part="SUPPLY7" gate="G$1" x="198.12" y="63.5"/>
<instance part="SUPPLY8" gate="G$1" x="152.4" y="96.52"/>
<instance part="GND1" gate="1" x="152.4" y="88.9"/>
<instance part="GND2" gate="1" x="7.62" y="106.68"/>
<instance part="GND3" gate="1" x="170.18" y="96.52"/>
<instance part="GND4" gate="1" x="172.72" y="76.2"/>
<instance part="GND5" gate="1" x="177.8" y="25.4"/>
<instance part="GND6" gate="1" x="147.32" y="55.88"/>
<instance part="GND7" gate="1" x="53.34" y="73.66"/>
<instance part="GND8" gate="1" x="53.34" y="60.96"/>
<instance part="GND9" gate="1" x="38.1" y="48.26"/>
</instances>
<busses>
</busses>
<nets>
<net name="D13" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="SCK"/>
<pinref part="U1" gate="G$1" pin="D13"/>
<wire x1="55.88" y1="119.38" x2="81.28" y2="119.38" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D12" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="MISO"/>
<pinref part="U1" gate="G$1" pin="D12"/>
<wire x1="55.88" y1="116.84" x2="81.28" y2="116.84" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D11" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="MOSI"/>
<pinref part="U1" gate="G$1" pin="D11"/>
<wire x1="55.88" y1="114.3" x2="81.28" y2="114.3" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D10" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="SDA"/>
<pinref part="U1" gate="G$1" pin="D10"/>
<wire x1="55.88" y1="111.76" x2="81.28" y2="111.76" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D9" class="0">
<segment>
<pinref part="U2" gate="G$1" pin="RST"/>
<pinref part="U1" gate="G$1" pin="D9"/>
<wire x1="55.88" y1="109.22" x2="81.28" y2="109.22" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D6" class="0">
<segment>
<pinref part="R1" gate="G$1" pin="1"/>
<pinref part="U1" gate="G$1" pin="D6"/>
<wire x1="20.32" y1="76.2" x2="12.7" y2="76.2" width="0.1524" layer="91"/>
<wire x1="12.7" y1="76.2" x2="12.7" y2="101.6" width="0.1524" layer="91"/>
<wire x1="12.7" y1="101.6" x2="81.28" y2="101.6" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D7" class="0">
<segment>
<pinref part="R2" gate="G$1" pin="1"/>
<pinref part="U1" gate="G$1" pin="D7"/>
<wire x1="20.32" y1="63.5" x2="10.16" y2="63.5" width="0.1524" layer="91"/>
<wire x1="10.16" y1="63.5" x2="10.16" y2="104.14" width="0.1524" layer="91"/>
<wire x1="10.16" y1="104.14" x2="81.28" y2="104.14" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D8" class="0">
<segment>
<pinref part="SG1" gate="G$1" pin="1"/>
<pinref part="U1" gate="G$1" pin="D8"/>
<wire x1="22.86" y1="50.8" x2="5.08" y2="50.8" width="0.1524" layer="91"/>
<wire x1="5.08" y1="50.8" x2="5.08" y2="106.68" width="0.1524" layer="91"/>
<wire x1="5.08" y1="106.68" x2="81.28" y2="106.68" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U6" gate="G$1" pin="G16"/>
<wire x1="185.42" y1="38.1" x2="180.34" y2="38.1" width="0.1524" layer="91"/>
<wire x1="180.34" y1="38.1" x2="180.34" y2="20.32" width="0.1524" layer="91"/>
<wire x1="180.34" y1="20.32" x2="60.96" y2="20.32" width="0.1524" layer="91"/>
<wire x1="60.96" y1="20.32" x2="60.96" y2="106.68" width="0.1524" layer="91"/>
<junction x="60.96" y="106.68"/>
</segment>
</net>
<net name="D5" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="R1"/>
<pinref part="U1" gate="G$1" pin="D5"/>
<wire x1="180.34" y1="165.1" x2="73.66" y2="165.1" width="0.1524" layer="91"/>
<wire x1="73.66" y1="165.1" x2="73.66" y2="99.06" width="0.1524" layer="91"/>
<wire x1="73.66" y1="99.06" x2="81.28" y2="99.06" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D4" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="R2"/>
<pinref part="U1" gate="G$1" pin="D4"/>
<wire x1="180.34" y1="162.56" x2="71.12" y2="162.56" width="0.1524" layer="91"/>
<wire x1="71.12" y1="162.56" x2="71.12" y2="96.52" width="0.1524" layer="91"/>
<wire x1="71.12" y1="96.52" x2="81.28" y2="96.52" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D3" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="R3"/>
<pinref part="U1" gate="G$1" pin="D3"/>
<wire x1="180.34" y1="160.02" x2="68.58" y2="160.02" width="0.1524" layer="91"/>
<wire x1="68.58" y1="160.02" x2="68.58" y2="93.98" width="0.1524" layer="91"/>
<wire x1="68.58" y1="93.98" x2="81.28" y2="93.98" width="0.1524" layer="91"/>
</segment>
</net>
<net name="D2" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="R4"/>
<pinref part="U1" gate="G$1" pin="D2"/>
<wire x1="180.34" y1="157.48" x2="66.04" y2="157.48" width="0.1524" layer="91"/>
<wire x1="66.04" y1="157.48" x2="66.04" y2="91.44" width="0.1524" layer="91"/>
<wire x1="66.04" y1="91.44" x2="81.28" y2="91.44" width="0.1524" layer="91"/>
</segment>
</net>
<net name="A3" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="C1"/>
<pinref part="U1" gate="G$1" pin="A3"/>
<wire x1="180.34" y1="152.4" x2="170.18" y2="152.4" width="0.1524" layer="91"/>
<wire x1="170.18" y1="152.4" x2="170.18" y2="111.76" width="0.1524" layer="91"/>
<wire x1="170.18" y1="111.76" x2="147.32" y2="111.76" width="0.1524" layer="91"/>
</segment>
</net>
<net name="A2" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="C2"/>
<pinref part="U1" gate="G$1" pin="A2"/>
<wire x1="180.34" y1="149.86" x2="167.64" y2="149.86" width="0.1524" layer="91"/>
<wire x1="167.64" y1="149.86" x2="167.64" y2="114.3" width="0.1524" layer="91"/>
<wire x1="167.64" y1="114.3" x2="147.32" y2="114.3" width="0.1524" layer="91"/>
</segment>
</net>
<net name="A1" class="0">
<segment>
<pinref part="U4" gate="G$1" pin="C3"/>
<pinref part="U1" gate="G$1" pin="A1"/>
<wire x1="180.34" y1="147.32" x2="165.1" y2="147.32" width="0.1524" layer="91"/>
<wire x1="165.1" y1="147.32" x2="165.1" y2="116.84" width="0.1524" layer="91"/>
<wire x1="165.1" y1="116.84" x2="147.32" y2="116.84" width="0.1524" layer="91"/>
</segment>
</net>
<net name="A0" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="A0"/>
<pinref part="U5" gate="G$1" pin="SIG"/>
<wire x1="147.32" y1="119.38" x2="160.02" y2="119.38" width="0.1524" layer="91"/>
<wire x1="160.02" y1="119.38" x2="160.02" y2="83.82" width="0.1524" layer="91"/>
<wire x1="160.02" y1="83.82" x2="180.34" y2="83.82" width="0.1524" layer="91"/>
</segment>
</net>
<net name="A4/SDA" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="A4/SDA"/>
<pinref part="U3" gate="G$1" pin="SDA"/>
<wire x1="147.32" y1="109.22" x2="177.8" y2="109.22" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U7" gate="G$1" pin="HV"/>
<wire x1="152.4" y1="71.12" x2="157.48" y2="71.12" width="0.1524" layer="91"/>
<wire x1="157.48" y1="71.12" x2="157.48" y2="109.22" width="0.1524" layer="91"/>
<junction x="157.48" y="109.22"/>
</segment>
</net>
<net name="A5/SCL" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="A5/SCL"/>
<pinref part="U3" gate="G$1" pin="SCL"/>
<wire x1="147.32" y1="106.68" x2="177.8" y2="106.68" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U7" gate="G$1" pin="HV1"/>
<wire x1="152.4" y1="68.58" x2="154.94" y2="68.58" width="0.1524" layer="91"/>
<wire x1="154.94" y1="68.58" x2="154.94" y2="106.68" width="0.1524" layer="91"/>
<junction x="154.94" y="106.68"/>
</segment>
</net>
<net name="CAM_SDA" class="0">
<segment>
<pinref part="U7" gate="G$1" pin="LV"/>
<pinref part="U6" gate="G$1" pin="G13"/>
<wire x1="193.04" y1="71.12" x2="190.5" y2="71.12" width="0.1524" layer="91"/>
<wire x1="190.5" y1="71.12" x2="190.5" y2="43.18" width="0.1524" layer="91"/>
<wire x1="190.5" y1="43.18" x2="185.42" y2="43.18" width="0.1524" layer="91"/>
</segment>
</net>
<net name="CAM_SCL" class="0">
<segment>
<pinref part="U7" gate="G$1" pin="LV1"/>
<pinref part="U6" gate="G$1" pin="G12"/>
<wire x1="193.04" y1="68.58" x2="187.96" y2="68.58" width="0.1524" layer="91"/>
<wire x1="187.96" y1="68.58" x2="187.96" y2="40.64" width="0.1524" layer="91"/>
<wire x1="187.96" y1="40.64" x2="185.42" y2="40.64" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$1" class="0">
<segment>
<pinref part="R1" gate="G$1" pin="2"/>
<pinref part="LED1" gate="G$1" pin="A"/>
<wire x1="30.48" y1="76.2" x2="38.1" y2="76.2" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$2" class="0">
<segment>
<pinref part="R2" gate="G$1" pin="2"/>
<pinref part="LED2" gate="G$1" pin="A"/>
<wire x1="30.48" y1="63.5" x2="38.1" y2="63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="+5V" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="5V"/>
<pinref part="SUPPLY1" gate="G$1" pin="VCC"/>
<wire x1="147.32" y1="99.06" x2="152.4" y2="99.06" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="VCC"/>
<pinref part="SUPPLY2" gate="G$1" pin="VCC"/>
<wire x1="177.8" y1="101.6" x2="170.18" y2="101.6" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U5" gate="G$1" pin="VCC"/>
<pinref part="SUPPLY3" gate="G$1" pin="VCC"/>
<wire x1="180.34" y1="81.28" x2="172.72" y2="81.28" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U6" gate="G$1" pin="5V"/>
<pinref part="SUPPLY4" gate="G$1" pin="VCC"/>
<wire x1="185.42" y1="30.48" x2="177.8" y2="30.48" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U7" gate="G$1" pin="HVCC"/>
<pinref part="SUPPLY5" gate="G$1" pin="VCC"/>
<wire x1="152.4" y1="63.5" x2="147.32" y2="63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="+3V3" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="3V3"/>
<pinref part="SUPPLY8" gate="G$1" pin="V3V3"/>
<wire x1="147.32" y1="96.52" x2="152.4" y2="96.52" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="3V3"/>
<pinref part="SUPPLY6" gate="G$1" pin="V3V3"/>
<wire x1="15.24" y1="119.38" x2="7.62" y2="119.38" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U7" gate="G$1" pin="LVCC"/>
<pinref part="SUPPLY7" gate="G$1" pin="V3V3"/>
<wire x1="193.04" y1="63.5" x2="198.12" y2="63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="GND"/>
<pinref part="GND1" gate="1" pin="GND"/>
<wire x1="147.32" y1="91.44" x2="152.4" y2="91.44" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="GND"/>
<pinref part="GND2" gate="1" pin="GND"/>
<wire x1="15.24" y1="109.22" x2="7.62" y2="109.22" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U3" gate="G$1" pin="GND"/>
<pinref part="GND3" gate="1" pin="GND"/>
<wire x1="177.8" y1="99.06" x2="170.18" y2="99.06" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U5" gate="G$1" pin="GND"/>
<pinref part="GND4" gate="1" pin="GND"/>
<wire x1="180.34" y1="78.74" x2="172.72" y2="78.74" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U6" gate="G$1" pin="GND"/>
<pinref part="GND5" gate="1" pin="GND"/>
<wire x1="185.42" y1="27.94" x2="177.8" y2="27.94" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U7" gate="G$1" pin="GND"/>
<pinref part="GND6" gate="1" pin="GND"/>
<wire x1="152.4" y1="58.42" x2="147.32" y2="58.42" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="LED1" gate="G$1" pin="C"/>
<pinref part="GND7" gate="1" pin="GND"/>
<wire x1="48.26" y1="76.2" x2="53.34" y2="76.2" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="LED2" gate="G$1" pin="C"/>
<pinref part="GND8" gate="1" pin="GND"/>
<wire x1="48.26" y1="63.5" x2="53.34" y2="63.5" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="SG1" gate="G$1" pin="2"/>
<pinref part="GND9" gate="1" pin="GND"/>
<wire x1="33.02" y1="50.8" x2="38.1" y2="50.8" width="0.1524" layer="91"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
