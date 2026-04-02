import numpy as np
import pickle
import os
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import cv2

# --- Setup ---
BaseOptions = mp.tasks.BaseOptions
HandLandmarker = mp.tasks.vision.HandLandmarker
HandLandmarkerOptions = mp.tasks.vision.HandLandmarkerOptions

options = HandLandmarkerOptions(
    base_options=BaseOptions(model_asset_path='hand_landmarker.task'),
    running_mode=mp.tasks.vision.RunningMode.IMAGE,  # IMAGE mode for static files
    num_hands=1,
    min_hand_detection_confidence=0.5)

def normalize_landmarks(landmarks):
    wrist = landmarks[0]
    coords = np.array([(lm.x - wrist.x, lm.y - wrist.y) for lm in landmarks])
    scale = np.linalg.norm(coords[9])
    if scale > 0:
        coords /= scale
    return coords.flatten()

GESTURES = ["palm", "back", "fist", "point"]

X, y = [], []

with HandLandmarker.create_from_options(options) as detector:
    for label, gesture in enumerate(GESTURES):
        folder = f"dataset/{gesture}"
        count = 0
        for fname in os.listdir(folder):
            img = cv2.imread(os.path.join(folder, fname))
            if img is None:
                continue
            img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=img_rgb)
            result = detector.detect(mp_image)
            if result.hand_landmarks:
                features = normalize_landmarks(result.hand_landmarks[0])
                X.append(features)
                y.append(label)
                count += 1
        print(f"{gesture}: {count} samples loaded")

X, y = np.array(X), np.array(y)
print(f"\nTotal dataset: {len(X)} samples")

# --- Training ---
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

clf = RandomForestClassifier(n_estimators=100, random_state=42)
clf.fit(X_train, y_train)

print(classification_report(y_test, clf.predict(X_test), target_names=GESTURES))

# --- Saving ---
with open("gesture_clf.pkl", "wb") as f:
    pickle.dump((clf, GESTURES), f)
print("Saved gesture_clf.pkl")