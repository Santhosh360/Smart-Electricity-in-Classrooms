#Constants
ipAddress = "192.168.43.187" #IP address of nodeMCU

initCounter = 5     #Used to initialize counter
                    #The value of initCounter denotes how much time it has 
                    #to wait to turn the electric appliances off after no person were detected. 

avgThreshold = 6.0  # This threshold denotes average of all pixels below 
                    #which object detection is impossible.

thresholdScore = 0.3#Used to draw rectangles around the person if its detection accuracy is above 30%

ledState = 0
counter = 5
personNotDetected = 1
exitThread = 0      #This variable is used to end all the threads if main thread is terminated.
hostAvaliable = 0
import threading
import hashlib
import numpy as np
import os
import six.moves.urllib as urllib
import sys
import tarfile
import tensorflow as tf
import zipfile
import cv2
from skimage import io
import numpy as np
from datetime import datetime

import time
from collections import defaultdict
from io import StringIO
from matplotlib import pyplot as plt
from PIL import Image
from utils import label_map_util
from utils import visualization_utils as vis_util
state = 0

def checkHost():
    global hostAvaliable
    while True:
        if exitThread == 1:
            exit()
        response = os.system("ping -n 1 " + ipAddress)
        if response == 0:
            hostAvaliable = 1
            print(ipAddress, 'is up!')
        else:
            hostAvaliable = 0
            print(ipAddress, 'is down!')
        time.sleep(1)    

def ledControl():
    global hostAvaliable
    global ledState
    while True:
        if exitThread == 1:
            now = datetime.now()
            hr = int(now.strftime("%H"))
            mn = int(now.strftime("%M"))
            htime = str(hr)+":"+str(mn)
            g2off="2"+htime+"off"
            try:
                urllib.request.urlopen('http://'+ipAddress+'/'+hashlib.md5(g2off.encode('utf-8')).hexdigest())
            except:
                print("Host unreachable...\n")
            exit()
        if hostAvaliable == 1:
            now = datetime.now()
            hr = int(now.strftime("%H"))
            mn = int(now.strftime("%M"))
            htime = str(hr)+":"+str(mn)
            if ledState == 1:
                g2on="2"+htime+"on"
                try:
                    urllib.request.urlopen('http://'+ipAddress+'/'+hashlib.md5(g2on.encode('utf-8')).hexdigest())
                except:
                    print("Host unreachable...\n")
            else:
                g2off="2"+htime+"off"
                try:
                    urllib.request.urlopen('http://'+ipAddress+'/'+hashlib.md5(g2off.encode('utf-8')).hexdigest())
                except:
                    print("Host unreachable...\n")

def resetCounter():
    global exitThread
    global personNotDetected
    global counter
    global initCounter
    global ledState
    while True:
        if exitThread == 1:
            exit()
        if personNotDetected == 1:
            counter = counter-1
            time.sleep(1)
            if counter == 0:
                ledState = 0
        else:
            if counter != initCounter:
                counter = initCounter
            ledState = 1
            time.sleep(1)
        print("Person not detected:",personNotDetected,"\n")  
        print("LED Status:",ledState,"\n")
    
t1 = threading.Thread(target=resetCounter)
t1.start()
t2 = threading.Thread(target=checkHost)
t2.start()
t3 = threading.Thread(target=ledControl)
t3.start()
def clear():
    os.system( 'cls' ) 
noto = cv2.imread("not.jpg")     

cap = cv2.VideoCapture(0)

MODEL_NAME = 'ssdlite_mobilenet_v2_coco_2018_05_09'
#MODEL_NAME = 'faster_rcnn_inception_v2_coco_2018_01_28'
#MODEL_NAME = 'ssd_mobilenet_v1_fpn_shared_box_predictor_640x640_coco14_sync_2018_07_03'
#MODEL_NAME = 'ssd_resnet50_v1_fpn_shared_box_predictor_640x640_coco14_sync_2018_07_03'

# Path to frozen detection graph. This is the actual model that is used for the object detection.
PATH_TO_CKPT = MODEL_NAME + '/frozen_inference_graph.pb'

# List of the strings that is used to add correct label for each box.
PATH_TO_LABELS = os.path.join('data', 'mscoco_label_map.pbtxt')

NUM_CLASSES = 90


detection_graph = tf.Graph()
with detection_graph.as_default():
    od_graph_def = tf.GraphDef()
    with tf.gfile.GFile(PATH_TO_CKPT, 'rb') as fid:
        serialized_graph = fid.read()
        od_graph_def.ParseFromString(serialized_graph)
        tf.import_graph_def(od_graph_def, name='')


label_map = label_map_util.load_labelmap(PATH_TO_LABELS)
categories = label_map_util.convert_label_map_to_categories(
    label_map, max_num_classes=NUM_CLASSES, use_display_name=True)
category_index = label_map_util.create_category_index(categories)


# Helper code
def load_image_into_numpy_array(image):
    (im_width, im_height) = image.size
    return np.array(image.getdata()).reshape(
        (im_height, im_width, 3)).astype(np.uint8)


# Detection
with detection_graph.as_default():
    with tf.Session(graph=detection_graph) as sess:
        while True:
            
            # Read frame from camera
            ret, image_np = cap.read()
            #print(np.mean(image_np))
            if np.mean(image_np) >= avgThreshold:
                if state == -1:
                    clear()
                state = 1
                #image_np = cv2.resize(image_np,(480,270))
                # Expand dimensions since the model expects images to have shape: [1, None, None, 3]
                image_np_expanded = np.expand_dims(image_np, axis=0)
                # Extract image tensor
                image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')
                # Extract detection boxes
                boxes = detection_graph.get_tensor_by_name('detection_boxes:0')
                # Extract detection scores
                scores = detection_graph.get_tensor_by_name('detection_scores:0')
                # Extract detection classes
                classes = detection_graph.get_tensor_by_name('detection_classes:0')
                # Extract number of detectionsd
                num_detections = detection_graph.get_tensor_by_name(
                    'num_detections:0')
                # Actual detection.
                (boxes, scores, classes, num_detections) = sess.run(
                    [boxes, scores, classes, num_detections],
                    feed_dict={image_tensor: image_np_expanded})
                # Visualization of the results of a detection.
                vis_util.visualize_boxes_and_labels_on_image_array(
                    image_np,
                    np.squeeze(boxes),
                    np.squeeze(classes).astype(np.int32),
                    np.squeeze(scores),
                    category_index,
                    use_normalized_coordinates=True,
                    line_thickness=8)
                no_of_persons = 0
                for i in range(len(np.squeeze(scores))):
                    if np.squeeze(scores)[i] > thresholdScore:
                        if np.squeeze(classes).astype(np.int32)[i] == 1:
                            no_of_persons = no_of_persons + 1
                #print("Person not detected:",personNotDetected,"\n")
                if no_of_persons == 0:
                    personNotDetected = 1
                else:
                    personNotDetected = 0    
                # Display output
                cv2.imshow('object detection', cv2.resize(image_np, (800, 600)))

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    cv2.destroyAllWindows()
                    exitThread = 1
                    break
            else:
                if state == 1:
                    personNotDetected = 1
                    print("Detection not possible...\n")  
                    cv2.imshow('object detection', cv2.resize(noto, (800, 600)))
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        cv2.destroyAllWindows()
                        exitThread = 1
                        break
                state = -1    