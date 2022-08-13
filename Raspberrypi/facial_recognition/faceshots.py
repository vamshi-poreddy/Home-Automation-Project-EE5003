import cv2

name = 'Vamshi' #Name of the person taking picture must be same name as folder in dataset

cam = cv2.VideoCapture(0)

cv2.namedWindow("press space to take a photo", cv2.WINDOW_NORMAL)#Opening window with camera
cv2.resizeWindow("press space to take a photo", 500, 300)#size of the window

img_counter = 0

while True:
    ret, frame = cam.read() #Reading the frames from camera
    if not ret:
        print("failed to grab frame")
        break
    cv2.imshow("press space to take a photo", frame)

    k = cv2.waitKey(1) #waiting forkeyboard key to be pressed
    if k%256 == 27:#Exit if Esc key is pressed
        # ESC pressed
        print("Escape hit, closing...")
        break
    elif k%256 == 32:#space to take capture the frame
        # SPACE pressed
        img_name = "dataset/"+ name +"/image_{}.jpg".format(img_counter)
        cv2.imwrite(img_name, frame) #Captured frame and asved in dataset folder
        print("{} written!".format(img_name))
        img_counter += 1

cam.release()

cv2.destroyAllWindows()
