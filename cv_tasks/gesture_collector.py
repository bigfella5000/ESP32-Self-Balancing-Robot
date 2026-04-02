import cv2
import os
import time

gesture_name = "point"  # CHANGE FOR EACH GESTURE
save_dir = f"dataset/{gesture_name}"
os.makedirs(save_dir, exist_ok=True)

cap = cv2.VideoCapture(0)

count = 0
last_save = 0
SAVE_INTERVAL = 0.1 # Save 10 frames per second while holding spacebar

while cap.isOpened():
    success, frame = cap.read()
    if not success: break

    cv2.putText(frame, f"Gesture: {gesture_name} | Saved: {count}",
                (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    cv2.imshow("Collect", frame)

    key = cv2.waitKey(1) & 0xFF
    now = time.time()
    if key == ord(' ') and now - last_save > SAVE_INTERVAL:
        path = os.path.join(save_dir, f"{gesture_name}_{count:04d}.jpg")
        cv2.imwrite(path, frame)
        count += 1
        last_save = now
        print(f"Saved {count}")
    elif key == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()