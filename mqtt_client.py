# Get data from TTN Console using Python

import paho.mqtt.client as mqtt
import json
import base64

APPEUI = '70B3D57ED001562C'
APPID  = 'forest_fire_detector'
PSW    = 'ttn-account-v2.iiB87XjxBryBkdnK7m-RZKTETclChdrQZVbNgUHd0Wg'

MQTTTOPIC = "forest_fire_detector/devices/alastair/down"
#MQTTTOPIC = "forest_fire_detector/devices/john/down"
MQTTMSG = json.dumps({"payload_fields":{"led":False, "newfield":4332, "field3":3353454}, "port":1})

def on_publish(client, userdata, mid):
    print("message published")

def on_connect(client, userdata, flags, rc):
    if rc==0:
        print("connected OK Returned code=",rc)
    else:
        print("Bad connection Returned code=",rc)
    #client.subscribe('forest_fire_detector/devices/john/up'.format(APPEUI))
    client.subscribe('forest_fire_detector/devices/alastair/up'.format(APPEUI))

def on_message(client, userdata, msg):
    client.publish(MQTTTOPIC, MQTTMSG)
    j_msg = json.loads(msg.payload.decode('utf-8'))
    dev_eui = j_msg['hardware_serial']
    dev_id = j_msg['dev_id']
    tmp = j_msg['payload_fields']['temperature']
    hum = j_msg['payload_fields']['humidity']
    lat = j_msg['payload_fields']['latitude']
    long = j_msg['payload_fields']['longitude']
    print('device:', dev_id, '| measurements: tmp =', tmp, 'hum =', hum, 'lat =', lat, 'long =', long)

# set paho.mqtt callback
client = mqtt.Client()
client.on_publish = on_publish
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set(APPID, PSW)
client.connect("us-west.thethings.network", 1883, 60) #MQTT port over TLS

try:
    client.loop_forever()
except KeyboardInterrupt:
    print('disconnect')
    client.disconnect()
