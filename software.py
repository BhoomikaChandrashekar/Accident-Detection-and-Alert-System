import urllib.request  as urllib2
import json
import time
import requests
from flask import Flask, redirect, render_template, request, session, url_for
from googleplaces import GooglePlaces, types, lang 
from twilio.rest import Client
import smtplib 
from email.mime.multipart import MIMEMultipart 
from email.mime.text import MIMEText 
from email.mime.base import MIMEBase 
from email import encoders
from math import *
from decimal import Decimal
import pymysql
import email
import imaplib

#function to find the distance between two coordinate points
def haversine_distance(lat2,long2,lat1,long1):
    slat=radians(lat1)
    slon=radians(long1)
    elat=radians(lat2)
    elon=radians(long2)
    dist = 6371.0710  * acos(sin(slat)*sin(elat) + cos(slat)*cos(elat)*cos(slon - elon))
    return dist

#function for database connectivity
def database_fetch(i,lat1,long1):
    connection = pymysql.connect(host="localhost",user="root",passwd="",database="miniproject_hospital" )
    cursor = connection.cursor()

    #executing the quires
    cursor.execute('Select * from hospitaldetails where name = %s',i)
    rows = cursor.fetchall()
    for row in rows:
        print('Hospital Name : ',i)
        print('Phone Number : ',row[1])
        print('EmailID : ',row[2])
        phno=row[1]
        voice_call(phno)
        print(phno)
        ID=row[2]
        mail(ID,lat1,long1)
    connection.close()
    
#function to call the nearest hospital
def voice_call(a):
    account_sid = 'XXXXXXXXX' #add account sid from twilio account
    auth_token = 'YYYYYYYYYY' #add account token from twilio account
    client = Client(account_sid, auth_token)

    call = client.calls.create(
                        twiml='<Response ><Say voice="alice">Emergency....Accident has been detected near to your hospital Check your official mail for location Thank you</Say></Response>',
                        to='+91'+a, #hospital phone number
                        from_='' #twilio number      
               )    
    print('Call SID number : ',call.sid)

#function to send mail
def mail(ID,lat1,long1):
    fromaddr = "xx@gmail.com" #sender's mail address
    toaddr = ID
    msg = MIMEMultipart() 
    msg['From'] = fromaddr 
    msg['To'] = toaddr 
    msg['Subject'] = "Accident location"
    url="http://www.google.com/maps/place/"+str(lat1)+","+str(long1)
    body = "Latitude : "+str(lat1)+"\nLongitude : "+str(long1)+"\nClick on the link for directions\n"+url
    msg.attach(MIMEText(body, 'plain')) 
    s = smtplib.SMTP('smtp.gmail.com', 587) 
    s.starttls() 
    s.login(fromaddr,"****") #sender's gmail password
    text = msg.as_string() 
    s.sendmail(fromaddr, toaddr, text)  
    print('Email sent')
    s.quit() 

#function to locate nearby hospitals and find nearest among those
def map(lat1,long1):
    API_KEY = 'xxxxxx' #google cloud api key for maps
    google_places = GooglePlaces(API_KEY) 
    query_result = google_places.nearby_search(  
    lat_lng ={'lat':lat1 , 'lng':long1 }, 
    radius = 500, 
    types =[types.TYPE_HOSPITAL]) 
    if query_result.has_attributions: 
        print (query_result.html_attributions) 
    dict1={}
    for place in query_result.places: 
        s=""
        if 'Hospital'.lower() in place.name.lower() and 'diagnostics'.lower() not in place.name.lower() and 'dental'.lower() not in place.name.lower() and 'eye'.lower() not in place.name.lower(): 
            s=place.name
            distance=haversine_distance(place.geo_location['lat'],place.geo_location['lng'],lat1,long1)
            dict1[s]=distance
    print(dict1)
    val = dict1.values()
    minval = min(val)
    for i in dict1:
        if dict1[i]== minval:
            j=i
    database_fetch(j,lat1,long1)


#code to fetch coordinates from cloud
READ_API_KEY='yyyyy' #thingspeak api key
CHANNEL_ID= 'qqqqqq' #thingspeak channel id 
lon=0
lat=0
while(True):
    conn = urllib2.urlopen("http://api.thingspeak.com/channels/%s/feeds/last.json?api_key=%s" \
                                % (CHANNEL_ID,READ_API_KEY))
    response = conn.read()
    data=json.loads(response)
    # print(data)
    lat1=float(data['field4']) #Field number from thingspeak account
    long1=float(data['field5']) #Field number from thingspeak account
    if lat1!=lat and long1!=lon:
        map(lat1,long1)
        lat=lat1
        lon=long1
    conn.close()
    
