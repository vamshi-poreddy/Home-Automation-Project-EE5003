# import the necessary packages
from imutils.video import VideoStream
from imutils.video import FPS
import face_recognition
import imutils
import pickle
import time
import cv2
from googleapiclient.http import MediaFileUpload
from Google import Create_Service
import smtplib

#function for Uploading the image to drive
def drive_upload(img_name):
    cf = '/home/pi/client_secret.json'#Authentication file from google OAuth
    apin = 'drive'
    apiver = 'v3'
    #adress of drive based on google API
    Scopes = ['https://www.googleapis.com/auth/drive']

    service = Create_Service(cf,apin,apiver,Scopes)

    folder_id='1lxOqdwSpn1pIMLfmBi0yZNXg0b5Hd6o_'#ID of the google drive folder
    file_name = img_name
    mime_type = 'image/jpeg'
    file_metadata = {'name':file_name,'parents':[folder_id]}
    media = MediaFileUpload('{0}'.format(file_name),mimetype=mime_type)#formatting the media file with type of data

    #uploading the file to specified drive folder
    service.files().create(body=file_metadata, media_body=media, fields='id').execute()

def send_email():
    #setting up the port and service address using smtp package
    smtp = smtplib.SMTP('smtp.gmail.com',587)
    smtp.ehlo()
    #starting the tls protocol for the smtp server
    smtp.starttls()
    smtp.ehlo()
    #logging into email using email address and apppassword
    smtp.login(email_add, "qurucfqrrrgjfvmh")
    subject = "Alert"
    body = "Intruder detected! Please check the images uploaded to drive"
    msg = "Subject: {}\n\n{}".format(subject,body)
    #sending the email with subject and body
    smtp.sendmail(email_add,email_add, msg)
    print("Message Sent")
    smtp.quit()
    
email_add = "poreddyvamshidharreddy@gmail.com" #sender and reciever email address same in my case
#email_pass = os.environ.get('Email_Pass') #password obtained from os environment variable for privacy
#Initialize 'currentname' to trigger only when a new person is identified.
currentname = "Unknown"
#Determine faces from encodings.pickle file model created from train_model.py
encodingsP = "encodings.pickle"

# load the known faces and embeddings along with OpenCV's Haar
# cascade for face detection
print("[INFO] loading encodings + face detector...")
data = pickle.loads(open(encodingsP, "rb").read())

# initialize the video stream and allow the camera sensor to warm up
# Set the ser to the followng
# src = 0 : for the build in single web cam, could be your laptop webcam
# src = 2 : I had to set it to 2 inorder to use the USB webcam attached to my laptop
vs = VideoStream(src=0,framerate=10).start() #for raspberry pi code tested for src=0 
time.sleep(2.0)
prev_sec = 0;

# start the FPS counter
fps = FPS().start()

# loop over frames from the video file stream
while True:
    #grab the frame from the threaded video stream and resize it
    # to 500px (to speedup processing)
    frame = vs.read()
    frame = imutils.resize(frame, width=500)
    # Detect the fce boxes
    boxes = face_recognition.face_locations(frame)
    # compute the facial embeddings for each face bounding box
    encodings = face_recognition.face_encodings(frame, boxes)
    names = []

    # loop over the facial embeddings
    for encoding in encodings:
        # attempt to match each face in the input image to our trained encodings
        matches = face_recognition.compare_faces(data["encodings"],encoding)
        name = "Unknown" #if face is not recognized, then print Unknown

        # check to see if we have found a match
        if True in matches:
            # find the indexes of all matched faces then initialize a
            # dictionary to count the total number of times each face
            # was matched
            matchedIdxs = [i for (i, b) in enumerate(matches) if b]
            counts = {}

            # loop over the matched indexes and maintain a count for
            # each recognized face
            for i in matchedIdxs:
                name = data["names"][i]
                counts[name] = counts.get(name, 0) + 1

            # determine the recognized face with the largest number
            # of votes (note: in the event of an unlikely tie Python
            # will select first entry in the dictionary)
            name = max(counts, key=counts.get)

            #If someone in your dataset is identified, print their name on the screen
        if currentname == name:
            #currentname = name
            print(currentname)
            #Take a picture to send in the email
            img_name = "image_{}.jpg".format(time.ctime(time.time()))
            cv2.imwrite(img_name, frame)
            #capturing the frame and loading it into img_name for drive upload
            print('Taking a picture.')
            request = drive_upload(img_name)
            #calling the dive upload function
            #To avoid spamming email is sent once every 60 sec but the interval can be changed
            if time.time()-prev_sec >= 60:
                prev_sec = time.time();
                send_email();
        
        # update the list of names
        names.append(name)

    # loop over the recognized faces
    for ((top, right, bottom, left), name) in zip(boxes, names):
        # draw the predicted face name on the image - color is in BGR
        cv2.rectangle(frame, (left, top), (right, bottom),(255, 128, 0), 2)
        y = top - 15 if top - 15 > 15 else top + 15
        cv2.putText(frame, name, (left, y), cv2.FONT_HERSHEY_SIMPLEX,.8, (0, 255, 255), 2)

    # display the image to our screen
    cv2.imshow("Facial Recognition is Running", frame)
    key = cv2.waitKey(1) & 0xFF

    # quit when 'q' key is pressed
    if key == ord("q"):
        break

    # update the FPS counter
    fps.update()

# stop the timer and display FPS information
fps.stop()
print("[INFO] elasped time: {:.2f}".format(fps.elapsed()))
print("[INFO] approx. FPS: {:.2f}".format(fps.fps()))

# Stopping the camera and closing the window
cv2.destroyAllWindows()
vs.stop()
