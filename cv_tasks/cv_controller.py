import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from ultralytics import YOLO
import pickle
import numpy as np
import time
import socket

UDP_IP = "192.168.1.129" # ESP32-WROOM-D's IP
UDP_PORT = 1234

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Setup Task API
BaseOptions = mp.tasks.BaseOptions
VisionRunningMode = mp.tasks.vision.RunningMode
HandLandmarkerOptions = mp.tasks.vision.HandLandmarkerOptions
HandLandmarker = mp.tasks.vision.HandLandmarker

# Dictionary for tracking
hand_state = {"is_detected": False, "landmarks": {}}

# Create detector
options = HandLandmarkerOptions(
    base_options=BaseOptions(model_asset_path='hand_landmarker.task'),
    running_mode=VisionRunningMode.VIDEO,
    num_hands=1,
    min_hand_detection_confidence=0.5)

# Import the person detection model
person_model = YOLO('yolov8n.pt')

# Import custom hand gesture classifier model
with open("gesture_clf.pkl", "rb") as f:
    clf, GESTURES = pickle.load(f)

def normalize_landmarks(landmarks):
    wrist = landmarks[0]
    coords = np.array([(lm.x - wrist.x, lm.y - wrist.y) for lm in landmarks])

    scale = np.linalg.norm(coords[9])
    if scale > 0:
        coords /= scale

    return coords.flatten()

with HandLandmarker.create_from_options(options) as hand_detector:
    ESP32_CAM_IP = "192.168.1.145"
    cap = cv2.VideoCapture(f"http://{ESP32_CAM_IP}/stream") # 0 for laptop's webcam, f"http://{ESP32_CAM_IP}/stream" for esp32-cam
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    TARGET_FPS = 10
    frame_interval = 1.0 / TARGET_FPS
    last_inference_time = 0
    while cap.isOpened():
        success, frame = cap.read()
        if not success: break

        now = time.time()
        if now - last_inference_time < frame_interval:
            continue
        last_inference_time = now

        # Run inference for 'person' (class 0)
        person_result = person_model.predict(frame, classes=[0], conf=0.5, verbose=False)[0] # The [0] is needed to grab the first and only result out of the batch prediction (only has one result since we're only passing a single frame in at a time)

        dx, dy = 0.0, 0.0
        for box in person_result.boxes:
            # Get coordinates
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            w, h = x2 - x1, y2 - y1
            
            # Draw the bounding box
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
            
            # Display coordinates and size in corner of box
            info_text = f"Pos: ({x1}, {y1}) Size: {w}x{h}"
            cv2.putText(frame, info_text, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

            (center_x, center_y) = (x1 + (x2 - x1) / 2, y1 + (y2 - y1) / 2) # The center coordinates of the person
            (dx, dy) = (center_x - frame.shape[1] / 2, center_y - frame.shape[0] / 2) # Distance person is from center of frame

        # Prepare image and timestamp
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame)
        frame_timestamp_ms = int(time.time() * 1000)
        # Run inference
        hand_result = hand_detector.detect_for_video(mp_image, frame_timestamp_ms)
        
        gesture_name = "none"
        if hand_result.hand_landmarks:
            hand_state["is_detected"] = True
            h, w, _ = frame.shape

            landmarks = hand_result.hand_landmarks[0]
            features = normalize_landmarks(landmarks).reshape(1, -1)

            gesture_idx = clf.predict(features)[0]
            confidence = clf.predict_proba(features)[0][gesture_idx]
            gesture_name = GESTURES[gesture_idx] if confidence > 0.8 else "none"
            print(f"{gesture_name} ({confidence:.2f})")
                
            for idx, lm in enumerate(landmarks):
                px, py = int(lm.x * w), int(lm.y * h)
                hand_state["landmarks"][idx] = (px, py)
                cv2.circle(frame, (px, py), 4, (0, 255, 0), cv2.FILLED)

        else:
            hand_state["is_detected"] = False

        # Send the data over to the motor controller
        message = str(f"{gesture_name},{dx},{dy}").encode()
        sock.sendto(message, (UDP_IP, UDP_PORT))


        cv2.imshow('Person and Hand Tracking', frame)
        if cv2.waitKey(1) & 0xFF == ord('q'): break

    cap.release()
    cv2.destroyAllWindows()


"""
        if hand_result.hand_landmarks:
            hand_state["is_detected"] = True
            h, w, _ = frame.shape
            num_hands = len(hand_result.hand_landmarks)

            for hand_idx in range(num_hands):
                landmarks = hand_result.hand_landmarks[hand_idx]
                    
                for idx, lm in enumerate(landmarks):
                    px, py = int(lm.x * w), int(lm.y * h)
                    hand_state["landmarks"][idx] = (px, py)
                    cv2.circle(frame, (px, py), 4, (0, 255, 0), cv2.FILLED)
                    

            if num_hands == 2:
                x1 = hand_result.hand_landmarks[0][8].x
                y1 = hand_result.hand_landmarks[0][8].y
                x2 = hand_result.hand_landmarks[1][8].x
                y2 = hand_result.hand_landmarks[1][8].y

                hand_distance = ((abs(x2 - x1)) ** 2 + (abs(y2 - y1)) ** 2) ** 0.5
"""